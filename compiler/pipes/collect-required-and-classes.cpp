// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-required-and-classes.h"

#include "compiler/const-manipulations.h"
#include "compiler/compiler-core.h"
#include "compiler/data/modulite-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/src-dir.h"
#include "compiler/function-pass.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"


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
      const auto &composer = G->get_composer_autoloader();
      if (const auto &psr4_filename = composer.psr4_lookup(file_name); !psr4_filename.empty()) {
        require_file(psr4_filename, false);
        return; // required from the composer autoload PSR-4 path
      }
      if (const auto &psr0_filename = composer.psr0_lookup(file_name); !psr0_filename.empty()) {
        auto file = require_file(psr0_filename, false);
        file->is_loaded_by_psr0 = true;
        return; // required from the composer autoload PSR-0 path
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
      if (f.type_hint != nullptr) {
        require_all_classes_in_phpdoc_type(f.type_hint);
      }
    });
    cur_class->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
      if (f.phpdoc) {
        require_all_classes_in_field_phpdoc(f.phpdoc);
      }
      if (f.type_hint != nullptr) {
        require_all_classes_in_phpdoc_type(f.type_hint);
      }
    });
  }

  // looking at /** @var Photo */ above a field, load Photo class
  inline void require_all_classes_in_field_phpdoc(const PhpDocComment *phpdoc) {
    if (const PhpDocTag *var_tag = phpdoc->find_tag(PhpDocType::var)) {
      if (auto tag_parsed = var_tag->value_as_type_and_var_name(current_function, current_function->genericTs)) {
        require_all_classes_in_phpdoc_type(tag_parsed.type_hint);
      }
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

  // SrcDir has `state_collect_required` atomic: every dir containing at least one PHP file is handled here
  void once_init_dir_atomic(SrcDirPtr dir) {
    auto expected = SrcDir::PassStatus::uninitialized;
    if (dir->state_collect_required.compare_exchange_strong(expected, SrcDir::PassStatus::processing)) {

      // ensure a parent dir has already been inited
      if (dir->parent_dir && dir->parent_dir->state_collect_required != SrcDir::PassStatus::done) {
        once_init_dir_atomic(dir->parent_dir);
      }
      // here is the moment of parsing .modulite.yaml inside a dir
      if (ModulitePtr modulite = load_modulite_inside_dir(dir)) {
        G->register_modulite(modulite);
        dir->nested_files_modulite = modulite;
        require_all_deps_of_modulite(modulite);
      }

    } else if (expected == SrcDir::PassStatus::processing) {
      while (dir->state_collect_required == SrcDir::PassStatus::processing) {
        std::this_thread::sleep_for(std::chrono::nanoseconds{100});
      }
    }
    dir->state_collect_required = SrcDir::PassStatus::done;
  }

  // parse .modulite.yaml inside a dir, called once per dir
  // it's somehow related to "collecting requires", so implemented here, to be done in parallel
  // moreover, symbols listed in "export" are used for classes collecting, see require_all_deps_of_modulite()
  // while parsing, cross-references like SomeClass or @another-m are not resolved, it will be done on all classes loaded
  ModulitePtr load_modulite_inside_dir(SrcDirPtr dir) {
    if (dir->has_composer_json) {
      // composer packages are implicit modulites "#vendorname/packagename" ("#" + json->name)
      // for instance, if a modulite in a project calls a function from a package, it's auto checked to be required
      // by default, all symbols from composer packages are exported ("exports" is empty, see modulite-check-rules.cpp)
      // but if it contains .modulite.yaml near composer.json, that yaml can manually declared exported functions
      ComposerJsonPtr composer_json = G->get_composer_json_at_dir(dir);
      kphp_assert(composer_json);
      ModulitePtr modulite = ModuliteData::create_from_composer_json(composer_json, dir->has_modulite_yaml);

      bool is_root = dir->full_dir_name == G->settings().composer_root.get();
      if (is_root || !modulite) {  // composer.json at project root is loaded, but not stored as a modulite
        return {};
      }
      dir->nested_files_modulite = modulite;
      return modulite;
    }

    if (dir->has_modulite_yaml) {
      // parse .modulite.yaml inside a regular dir
      // find the dir up the tree with .modulite.yaml; say, dir contains @msg/channels, with_parent expected to be @msg
      SrcDirPtr with_parent = dir;
      while (with_parent && with_parent != G->get_main_file()->dir && !with_parent->nested_files_modulite) {
        with_parent = with_parent->parent_dir;
      }
      ModulitePtr parent = with_parent ? with_parent->nested_files_modulite : ModulitePtr{};

      ModulitePtr modulite = ModuliteData::create_from_modulite_yaml(dir->get_modulite_yaml_filename(), parent);
      if (!modulite || modulite->modulite_name.empty()) {   // an error while parsing yaml, was already printed
        return {};
      }
      dir->nested_files_modulite = modulite;
      return modulite;
    }

    // a dir doesn't contain a modulite.yaml file itself — but if it's nested into another, propagate from the above
    // - VK/Messages/      it's dir->parent_dir, already inited
    //   .modulite.yaml    @messages, already parsed
    //   - Core/           it's dir, no .modulite.yaml => assign @messages here
    if (dir->parent_dir) {
      dir->nested_files_modulite = dir->parent_dir->nested_files_modulite;
    }
    return {};
  }

  // symbols listed in "export" in .modulite.yaml are also used for classes collecting
  // without this, classes unreachable via PHP code but listed in "export" lead to an error on names resolving
  // this is quite important, because Modulite plugin auto-generates config from all sources,
  // and some of them may not be reachable at the moment of compilation (some wip yet unused code)
  void require_all_deps_of_modulite(ModulitePtr modulite) {
    for (const ModuliteSymbol &s : modulite->exports) {
      if (s.kind == ModuliteSymbol::kind_ref_stringname) {
        vk::string_view e = s.ref_stringname;   // exported symbol: class / define / method / etc.
        bool seems_like_classname = !e.ends_with(")") && e[0] != '$' && e[0] != '@' && e[0] != '#';
        if (seems_like_classname) {
          require_class(modulite->modulite_namespace + e);
        }
      }
    }
  }

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream)
    : file_stream(file_stream)
    , function_stream(function_stream) {
  }

  std::string get_description() override {
    return "Collect required";
  }

  void on_start() override {
    if (current_function->type == FunctionData::func_class_holder) {
      require_all_deps_of_class(current_function->class_id);
    } else if (current_function->type == FunctionData::func_local) {
      require_all_classes_from_func_declaration(current_function);
    } else if (current_function->type == FunctionData::func_main) {
      SrcDirPtr dir = current_function->file_id->dir;
      if (dir->state_collect_required != SrcDir::PassStatus::done) {
        once_init_dir_atomic(dir);
      }
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
      std::string name = collect_string_concatenation(require->expr(), true);
      kphp_error_act (!name.empty(), "Not a string in 'require' arguments", return root);
      if (is_composer_autoload(name)) {
        return require_composer_autoload(root);
      }
      return make_require_call(root, name, require->once, require->builtin);
    }

    if (auto as_phpdoc_var = root.try_as<op_phpdoc_var>()) {
      require_all_classes_in_phpdoc_type(as_phpdoc_var->type_hint);
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
