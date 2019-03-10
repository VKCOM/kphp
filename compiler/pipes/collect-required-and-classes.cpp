#include "compiler/pipes/collect-required-and-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"

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
    for (const auto &dep : cur_class->str_dependents) {
      require_class(replace_characters(dep.class_name, '\\', '/'));
    }
    // значения констант класса могут содержать константы других классов, которым нужно require_class()
    cur_class->members.for_each([&](ClassMemberConstant &c) {
      c.value = run_function_pass(c.value, this, nullptr);
    });
    cur_class->members.for_each([&](ClassMemberStaticField &f) {
      f.init_val = run_function_pass(f.init_val, this, nullptr);
      if (f.phpdoc_token) {
        require_all_classes_in_phpdoc(f.phpdoc_token);
      }
    });
    cur_class->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.phpdoc_token) {
        require_all_classes_in_phpdoc(f.phpdoc_token);
      }
    });
  }

  // если /** @var Photo */ над полем инстанса, видим класс Photo даже если нет явного вызова конструктора
  inline void require_all_classes_in_phpdoc(const Token *phpdoc_token) {
    std::string param_var_name, type_str;
    if (PhpDocTypeRuleParser::find_tag_in_phpdoc(phpdoc_token->str_val, php_doc_tag::var, param_var_name, type_str, 0)) {
      PhpDocTypeRuleParser parser(current_function);
      parser.parse_from_type_string(type_str);
      for (const auto &class_name : parser.get_unknown_classes()) {
        require_class(replace_characters(class_name, '\\', '/'));
      }
    }
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
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_func_call && root->extra_type != op_ex_func_call_arrow) {
      require_function(get_full_static_member_name(current_function, root->get_string(), true));
    }

    if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
      size_t pos$$ = root->get_string().find("::");
      if (pos$$ != string::npos) {
        const string &class_name = root->get_string().substr(0, pos$$);
        require_class(resolve_uses(current_function, class_name, '/'));
      }
    }

    if (root->type() == op_constructor_call) {
      bool is_lambda = root->get_func_id() && root->get_func_id()->is_lambda();
      if (!is_lambda && likely(!root->type_help)) {     // type_help <=> Memcache | Exception
        require_class(resolve_uses(current_function, root->get_string(), '/'));
      }
    }

    return root;
  }

  VertexAdaptor<op_func_call> make_require_call(VertexPtr v) {
    kphp_error_act (v->type() == op_string, "Not a string in 'require' arguments", return {});
    if (SrcFilePtr file = require_file(v->get_string(), true)) {
      auto call = VertexAdaptor<op_func_call>::create();
      call->str_val = file->main_func_name;
      return call;
    }
    kphp_error (0, format("Cannot require [%s]\n", v->get_string().c_str()));
    return {};
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_require_once || root->type() == op_require) {
      kphp_assert_msg(root->size() == 1, "Only one child possible for require vertex");
      root->back() = make_require_call(root->back());
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
