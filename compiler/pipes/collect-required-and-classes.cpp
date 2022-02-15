// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-required-and-classes.h"

#include "common/wrappers/likely.h"

#include "compiler/const-manipulations.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"

class CollectRequiredPass final : public FunctionPassBase {
private:
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

  SrcFilePtr require_file(const std::string &file_name, bool error_if_not_exists, bool builtin = false) {
    return G->require_file(file_name, current_function->file_id->owner_lib, file_stream, error_if_not_exists, builtin);
  }

  void require_function(const std::string &name) {
    G->require_function(name, function_stream);
  }

  void require_class(const std::string &class_name) {
    // avoid a race condition, when we try to search for RpcFunction.php and other built-in classes that are visible from index.php
    // (if such files exist, extra src_xxx$called variables will be created: unstable codegeneration)
    while (!G->get_functions_txt_parsed()) {
      usleep(100000);
    }

    if (G->get_class(class_name)) {
      return; // no need to require extra files
    }
    std::string file_name = replace_characters(class_name, '\\', '/');

    if (G->settings().is_composer_enabled()) {
      const auto &psr4_filename = G->get_composer_autoloader().psr4_lookup(file_name);
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
    require_file(file_name + ".php", false);
  }

  inline void require_all_deps_of_class(ClassPtr cur_class) {
    if (cur_class->name == "FFI") {
      FFIRoot::register_builtin_classes(function_stream);
    }

    for (const auto &dep : cur_class->get_str_dependents()) {
      if (!cur_class->is_builtin() && dep.class_name == "Throwable") {
        if (cur_class->is_interface()) {
          kphp_error(false, fmt_format("Interface {} cannot extend Throwable", cur_class->name));
        } else {
          kphp_error(false, fmt_format("Class {} cannot implement Throwable, extend Exception instead", cur_class->name));
        }
      }

      require_class(dep.class_name);
    }
    // class constant values may contain other class constants that may need require_class()
    cur_class->members.for_each([&](ClassMemberConstant &c) {
      run_function_pass(c.value, this);
    });
    cur_class->members.for_each([&](ClassMemberStaticField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
      if (f.phpdoc) {
        require_all_classes_in_field_phpdoc(f.phpdoc);
      }
    });
    cur_class->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
      if (f.phpdoc) {
        require_all_classes_in_field_phpdoc(f.phpdoc);
      }
    });
  }

  // When looking at /** @var Photo */ under the instance field, assume that we
  // need to load the Photo class even if there is no explicit constructor call
  inline void require_all_classes_in_field_phpdoc(const PhpDocComment *phpdoc) {
    if (const PhpDocTag *type_and_var_name = phpdoc->find_tag(PhpDocType::var)) {
      // we can't use parse_type_and_var_name() here
      // since classes are not available at this point: we need to fetch these unknown classes
      std::vector<Token> tokens = phpdoc_to_tokens(type_and_var_name->value);
      auto cur_tok = tokens.cbegin();
      PhpDocTypeHintParser parser(current_function);
      const TypeHint *type_hint = parser.parse_from_tokens_silent(cur_tok);
      require_all_classes_in_phpdoc_type(type_hint);
    }
  }

  // Searching for classes inside @var/@param phpdoc as well as inside type hints
  inline void require_all_classes_in_phpdoc_type(const TypeHint *type_hint) {
    if (type_hint && type_hint->has_instances_inside()) {
      type_hint->traverse([this](const TypeHint *child) {
        if (const auto *as_instance = child->try_as<TypeHintInstance>()) {
          if (!as_instance->has_self_static_parent_inside()) {
            require_class(as_instance->full_class_name);
          }
        }
      });
    }
  }

  // Collect classes only from type hints in PHP code, as phpdocs @param/@return haven't been parsed up to this execution point.
  // TODO: shall we fix this?
  inline void require_all_classes_from_func_declaration(FunctionPtr f) {
    for (const auto &p: f->get_params()) {
      if (p.as<op_func_param>()->type_hint) {
        require_all_classes_in_phpdoc_type(p.as<op_func_param>()->type_hint);
      }
    }
    if (f->return_typehint) {
      require_all_classes_in_phpdoc_type(f->return_typehint);
    }
  }

  VertexPtr make_require_call(VertexPtr root, const std::string &name, bool once, bool builtin = false) {
    auto file = require_file(name, true, builtin);
    kphp_error_act (file, fmt_format("Cannot require [{}]\n", name), return root);
    VertexPtr call = VertexAdaptor<op_func_call>::create();
    call->set_string(file->main_func_name);
    call->location = root->location;
    if (once) {
      VertexPtr cond = VertexAdaptor<op_log_not>::create(file->get_main_func_run_var());
      call = VertexAdaptor<op_if>::create(cond, VertexAdaptor<op_seq>::create(call));
      call->location = root->location;
    }
    return call;
  }

  // whether required file name refers to the composer-generated autoload file
  static bool is_composer_autoload(const std::string &required_path) {
    // fast path: it there is no "/autoload.php", then we don't
    // need to bother and expand the filename
    if (!vk::ends_with(required_path, "/autoload.php")) {
      return false;
    }
    auto full_name = G->search_required_file(required_path);
    return G->get_composer_autoloader().is_autoload_file(full_name);
  }

  VertexPtr require_composer_autoload(VertexPtr root) {
    // generate a block that contains a sequence of require_once() calls
    // for all files that composer-generated autoload.php would require;
    //
    // note that we use once=true all the time since composer would
    // do the same (it uses require+inclusion map to perform the inclusion exactly once)
    const auto &files_to_require = G->get_composer_autoloader().get_files_to_require();
    std::vector<VertexPtr> list;
    list.reserve(files_to_require.size());
    for (const auto &file : files_to_require) {
      list.emplace_back(make_require_call(root, file, true));
    }
    return VertexAdaptor<op_seq>::create(list).set_location(root->location);
  }

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    file_stream(file_stream),
    function_stream(function_stream) {
  }

  std::string get_description() override {
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

    if (auto try_op = root.try_as<op_try>()) {
      for (auto v : try_op->catch_list()) {
        auto catch_op = v.as<op_catch>();
        require_class(catch_op->type_declaration);
      }
      return root;
    }

    if (root->type() == op_func_call && root->extra_type != op_ex_func_call_arrow) {
      size_t pos = root->get_string().find("::");
      if (pos != std::string::npos) {
        const std::string &prefix_name = root->get_string().substr(0, pos);
        const std::string &local_name = root->get_string().substr(pos + 2);
        const std::string &class_name = resolve_uses(current_function, prefix_name);
        require_class(class_name);
        require_function(resolve_static_method_append_context(current_function, prefix_name, class_name, local_name));
      } else {
        require_function(root->get_string());
      }
    }

    if (root->type() == op_var || root->type() == op_func_name) {
      size_t pos = root->get_string().find("::");
      if (pos != std::string::npos) {
        const std::string &prefix_name = root->get_string().substr(0, pos);
        const std::string &class_name = resolve_uses(current_function, prefix_name);
        require_class(class_name);
      }
    }

    if (auto alloc = root.try_as<op_alloc>()) {
      if (!alloc->allocated_class) {
        require_class(resolve_uses(current_function, alloc->allocated_class_name));
      }
    }

    if (auto require = root.try_as<op_require>()) {
      std::string name = collect_string_concatenation(require->expr());
      kphp_error_act (!name.empty(), "Not a string in 'require' arguments", return root);
      if (is_composer_autoload(name)) {
        return require_composer_autoload(root);
      }
      return make_require_call(root, name, require->once, require->builtin);
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
