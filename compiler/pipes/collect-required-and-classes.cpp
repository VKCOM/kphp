// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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

class CollectRequiredPass final : public FunctionPassBase {
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
    if (G->get_class(class_name)) {
      return; // no need to require extra files
    }

    if (G->settings().is_composer_enabled()) {
      const auto &psr4_filename = G->get_composer_class_loader().psr4_lookup(class_name);
      if (!psr4_filename.empty()) {
        require_file(psr4_filename, false);
        return; // required from the composer autoload PSR-4 path
      }
    }

    // fallback to the default class autoloading scheme;
    //
    // in case that PHP-file doesn't exist we don't give any error
    // as they'll be reported as missing classes errors later;
    // for builtin and non-autoloadable classes there will be no errors
    require_file(class_name + ".php", false);
  }

  inline void require_all_deps_of_class(ClassPtr cur_class) {
    for (const auto &dep : cur_class->get_str_dependents()) {
      require_class(replace_characters(dep.class_name, '\\', '/'));
    }
    // class constant values may contain other class constants that may need require_class()
    cur_class->members.for_each([&](ClassMemberConstant &c) {
      run_function_pass(c.value, this);
    });
    cur_class->members.for_each([&](ClassMemberStaticField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
      if (!f.phpdoc_str.empty()) {
        require_all_classes_in_field_phpdoc(f.phpdoc_str);
      }
    });
    cur_class->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
      if (!f.phpdoc_str.empty()) {
        require_all_classes_in_field_phpdoc(f.phpdoc_str);
      }
    });
  }

  // When looking at /** @var Photo */ under the instance field, assume that we
  // need to load the Photo class even if there is no explicit constructor call
  inline void require_all_classes_in_field_phpdoc(vk::string_view phpdoc_str) {
    if (auto type_and_var_name = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
      // we don't use phpdoc_parse_type_and_var_name() here since classes
      // are not available at this point and we need to fetch these unknown classes
      std::vector<Token> tokens = phpdoc_to_tokens(*type_and_var_name);
      std::vector<Token>::const_iterator cur_tok = tokens.begin();
      PhpDocTypeRuleParser parser(current_function);
      auto type_expr = parser.parse_from_tokens_silent(cur_tok);
      require_all_classes_in_phpdoc_type(type_expr);
    }
  }

  // Searching for classes inside @var/@param phpdoc as well as inside type hints
  inline void require_all_classes_in_phpdoc_type(VertexPtr type_expr) {
    std::function<void(VertexPtr)> find_unknown_and_require = [this, &find_unknown_and_require](VertexPtr type_expr) {
      if (auto as_op_class = type_expr.try_as<op_type_expr_class>()) {
        const std::string &class_name = resolve_uses(current_function, as_op_class->class_name, '\\');
        if (!G->get_class(class_name)) {
          require_class(replace_characters(class_name, '\\', '/'));
        }
      }

      for (auto i : *type_expr) {
        find_unknown_and_require(i);
      }
    };

    find_unknown_and_require(type_expr);
  }

  // Collect classes to require from the function prototype.
  // WARNING! We inspect only the type hints (called type declarations below).
  //          phpdoc with @param is ignored.
  //          TODO: fix this when we'll parse the phpdoc once.
  inline void require_all_classes_from_func_declaration(FunctionPtr f) {
    for (const auto &p: f->get_params()) {
      if (p.as<op_func_param>()->type_hint) {
        require_all_classes_in_phpdoc_type(p.as<op_func_param>()->type_hint);
      }
    }
    // f->return_typehint is ignored;
    // we assume that if it returns an instance, it'll reference the required class in one way or another
  }

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    file_stream(file_stream),
    function_stream(function_stream) {
  }

  string get_description() override {
    return "Collect required";
  }

  void on_start() override {
    if (current_function->type == FunctionData::func_class_holder) {
      require_all_deps_of_class(current_function->class_id);
    } else if (current_function->type == FunctionData::func_local) {
      require_all_classes_from_func_declaration(current_function);
    }
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
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
