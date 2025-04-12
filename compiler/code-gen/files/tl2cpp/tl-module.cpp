// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tl2cpp/tl-module.h"

#include "compiler/code-gen/files/tl2cpp/tl-constructor.h"
#include "compiler/code-gen/files/tl2cpp/tl-function.h"
#include "compiler/code-gen/files/tl2cpp/tl-type.h"

namespace tl2cpp {
std::unordered_map<std::string, Module> modules;
std::set<std::string> modules_with_functions;

void Module::compile_tl_h_file(CodeGenerator &W) const {
  W << OpenFile(name + ".h", "tl");
  W << "#pragma once" << NL;
  W << ExternInclude("tl/tl_const_vars.h");
  W << h_includes;
  W << NL;
  for (const auto &t : target_types) {
    for (const auto &constructor : t->constructors) {
      W << TlConstructorDecl(constructor.get());
    }
  }
  for (const auto &t : target_types) {
    W << TlTypeDeclaration(t);
    if (is_type_dependent(t, tl)) {
      W << TlTypeDefinition(t);
    }
  }
  for (const auto &t : target_types) {
    if (is_type_dependent(t, tl)) {
      for (const auto &constructor : t->constructors) {
        W << TlConstructorDef(constructor.get());
      }
    }
  }
  for (const auto &f : target_functions) {
    W << TlFunctionDecl(f);
  }
  W << CloseFile();
}

void Module::compile_tl_cpp_file(CodeGenerator &W) const {
  W << OpenFile(name + ".cpp", "tl", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  W << Include("tl/" + name + ".h") << NL;
  W << cpp_includes;

  for (const auto &t : target_types) {
    if (!is_type_dependent(t, tl)) {
      W << TlTypeDefinition(t);
      for (const auto &constructor : t->constructors) {
        W << TlConstructorDef(constructor.get());
      }
    }
  }
  for (const auto &f: target_functions) {
    W << TlFunctionDef(f);
  }
  W << CloseFile();
}

void Module::add_obj(const std::unique_ptr<vk::tlo_parsing::combinator> &f) {
  modules_with_functions.insert(name);
  target_functions.push_back(f.get());
  update_dependencies(f);

  if (TlFunctionDecl::does_tl_function_need_typed_fetch_store(f.get())) {
    ClassPtr klass = get_php_class_of_tl_function(f.get());
    kphp_assert(klass);
    h_includes.add_class_forward_declaration(klass);
    cpp_includes.add_class_include(klass);
    klass = get_php_class_of_tl_function_result(f.get());
    kphp_assert(klass);
    cpp_includes.add_class_include(klass);
  }
}

void Module::add_obj(const std::unique_ptr<vk::tlo_parsing::type> &t) {
  target_types.push_back(t.get());
  update_dependencies(t);

  for (const auto &c : t->constructors) {
    if (TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(c.get())) {
      auto php_classes = get_all_php_classes_of_tl_constructor(c.get());
      std::for_each(php_classes.begin(), php_classes.end(), [&](ClassPtr klass) { h_includes.add_class_include(klass); });
    }
  }
}

void Module::update_dependencies(const std::unique_ptr<vk::tlo_parsing::combinator> &combinator) {
  for (const auto &arg : combinator->args) {
    collect_deps_from_type_tree(arg->type_expr.get());
  }
  if (combinator->is_function()) {
    collect_deps_from_type_tree(combinator->result.get());
  }
}

void Module::update_dependencies(const std::unique_ptr<vk::tlo_parsing::type> &t) {
  for (const auto &c : t->constructors) {
    update_dependencies(c);
  }
}

void Module::collect_deps_from_type_tree(vk::tlo_parsing::expr_base *expr) {
  if (auto *as_type_expr = expr->as<vk::tlo_parsing::type_expr>()) {
    std::string expr_module = get_module_name(tl->types[as_type_expr->type_id]->name);
    if (name != expr_module) {
      h_includes.add_raw_filename_include("tl/" + expr_module + ".h");
    }
    for (const auto &child : as_type_expr->children) {
      collect_deps_from_type_tree(child.get());
    }
  } else if (auto *as_type_array = expr->as<vk::tlo_parsing::type_array>()) {
    for (const auto &arg : as_type_array->args) {
      collect_deps_from_type_tree(arg->type_expr.get());
    }
  }
}

bool Module::is_empty() const {
  if (G->get_untyped_rpc_tl_used()) {
    return false;
  }
  return
    std::all_of(target_functions.begin(), target_functions.end(), [](const auto *f) {
      return !TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);
    }) &&
    std::all_of(target_types.begin(), target_types.end(), [](const auto *t) {
      return !TlTypeDeclaration::does_tl_type_need_typed_fetch_store(t);
    });
}

}
