#include "compiler/pipes/collect-required-and-classes.h"

#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"

namespace {
std::string collect_string_concatenation(VertexPtr v) {
  if (auto string = v.try_as<op_string>()) {
    return string->str_val;
  }
  if (auto concat = v.try_as<op_concat>()) {
    auto left = collect_string_concatenation(concat->lhs());
    auto right = collect_string_concatenation(concat->rhs());
    return (left.empty() || right.empty()) ? std::string() : (left + right);
  }
  return std::string();
}
} // namespace

class CollectRequiredPass : public FunctionPassBase {
private:
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

  SrcFilePtr require_file(const string &file_name, bool error_if_not_exists) {
    return G->require_file(file_name, current_function->file_id->owner_lib, file_stream, error_if_not_exists);
  }

  void require_function(const string &name) {
    G->require_function(name, function_stream);
  }

  void require_class(const string &class_name) {
    if (!G->get_class(class_name)) {              // при отсутствии php-файла не ругаемся:
      require_file(class_name + ".php", false);   // ошибки об отсутствующих классах будут позже,
    }                                             // а у built-in и не-autoloadable классов файлов и нет
  }

  inline void require_all_deps_of_class(ClassPtr cur_class) {
    for (const auto &dep : cur_class->get_str_dependents()) {
      require_class(replace_characters(dep.class_name, '\\', '/'));
    }
    // значения констант класса могут содержать константы других классов, которым нужно require_class()
    cur_class->members.for_each([&](ClassMemberConstant &c) {
      c.value = run_function_pass(c.value, this);
    });
    cur_class->members.for_each([&](ClassMemberStaticField &f) {
      if (f.var->init_val) {
        f.var->init_val = run_function_pass(f.var->init_val, this);
      }
      if (!f.phpdoc_str.empty()) {
        require_all_classes_in_phpdoc(f.phpdoc_str);
      }
    });
    cur_class->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        f.var->init_val = run_function_pass(f.var->init_val, this);
      }
      if (!f.phpdoc_str.empty()) {
        require_all_classes_in_phpdoc(f.phpdoc_str);
      }
    });
  }

  // если /** @var Photo */ над полем инстанса, видим класс Photo даже если нет явного вызова конструктора
  inline void require_all_classes_in_phpdoc(const vk::string_view &phpdoc_str) {
    if (auto type_and_var_name = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
      require_all_classes_in_phpdoc_type(*type_and_var_name);
    }
  }

  // парсим валидный тип phpdoc — что внутри @var/@param/type declaration — и ищем классы
  inline void require_all_classes_in_phpdoc_type(const std::string &type_str) {
    // здесь вручную бьём на токены, а НЕ вызываем phpdoc_parse_type_and_var_name()
    // потому что классов пока нет, и нам как раз нужно извлечь неизвестные классы
    std::vector<Token> tokens = phpdoc_to_tokens(type_str.c_str(), type_str.size());
    std::vector<Token>::const_iterator cur_tok = tokens.begin();
    PhpDocTypeRuleParser parser(current_function);
    parser.parse_from_tokens_silent(cur_tok);

    for (const auto &class_name : parser.get_unknown_classes()) {
      require_class(replace_characters(class_name, '\\', '/'));
    }
  }

  // достаём и require'им все классы, исходя из прототипа функции
  // ВНИМАНИЕ! сейчас анализируется только type declaration, а phpdoc с @param'ами над функцией не анализируется
  //           (надо бы, но это когда мы phpdoc будем парсить единожды, а не постоянно при надобности)
  inline void require_all_classes_from_func_declaration(FunctionPtr f) {
    for (const auto &p: f->get_params()) {
      if (!p.as<op_func_param>()->type_declaration.empty()) {
        require_all_classes_in_phpdoc_type(p.as<op_func_param>()->type_declaration);
      }
    }
    // из f->return_typehint не достаём: считаем, что если функция возвращает класс, уж она-то его создаст
  }

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    file_stream(file_stream),
    function_stream(function_stream) {
  }

  string get_description() {
    return "Collect required";
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }

    if (function->type == FunctionData::func_class_holder) {
      require_all_deps_of_class(function->class_id);
    } else if (function->type == FunctionData::func_local) {
      require_all_classes_from_func_declaration(function);
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root) {
    stage::set_line(root->location.line);

    if (root->type() == op_func_call && root->extra_type != op_ex_func_call_arrow) {
      auto full_static_name = get_full_static_member_name(current_function, root->get_string(), true);
      if (!full_static_name.empty()) {
        require_function(full_static_name);
      }
    }

    if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
      size_t pos$$ = root->get_string().find("::");
      if (pos$$ != string::npos) {
        const string &class_name = root->get_string().substr(0, pos$$);
        require_class(resolve_uses(current_function, class_name, '/'));
      }
    }

    if (auto alloc = root.try_as<op_alloc>()) {
      if (!alloc->allocated_class) {
        require_class(resolve_uses(current_function, alloc->allocated_class_name, '/'));
      }
    }

    if (auto require = root.try_as<op_require>()) {
      string name = collect_string_concatenation(require->expr());
      kphp_error_act (!name.empty(), "Not a string in 'require' arguments", return root);
      auto file = require_file(name, true);
      kphp_error_act (file, fmt_format("Cannot require [{}]\n", name), return root);
      VertexPtr call = VertexAdaptor<op_func_call>::create();
      call->set_string(file->main_func_name);
      call->location = root->location;
      if (require->once) {
        VertexPtr cond = VertexAdaptor<op_log_not>::create(file->get_main_func_run_var());
        call = VertexAdaptor<op_if>::create(cond, VertexAdaptor<op_seq>::create(call));
        call->location = root->location;
      }
      return call;
    }

    return root;
  }

};


void CollectRequiredAndClassesF::execute(FunctionPtr function, CollectRequiredAndClassesF::OStreamT &os) {
  auto &ready_function_stream = *os.template project_to_nth_data_stream<0>();
  auto &file_stream = *os.template project_to_nth_data_stream<1>();
  auto &function_stream = *os.template project_to_nth_data_stream<2>();
  auto &class_stream = *os.template project_to_nth_data_stream<3>();

  CollectRequiredPass pass(file_stream, function_stream);
  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  if (function->type == FunctionData::func_class_holder) {
    class_stream << function->class_id;
  }

  ready_function_stream << function;
}
