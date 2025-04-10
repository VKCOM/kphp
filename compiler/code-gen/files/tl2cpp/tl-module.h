// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"
#include "compiler/code-gen/includes.h"

namespace tl2cpp {
class Module;

extern std::unordered_map<std::string, Module> modules;

extern std::set<std::string> modules_with_functions;

// A module is a two (.cpp+.h) files that correspond to one TL scheme module.
class Module {
public:
  std::vector<vk::tlo_parsing::type*> target_types;
  std::vector<vk::tlo_parsing::combinator*> target_functions;
  IncludesCollector h_includes;
  IncludesCollector cpp_includes;
  std::string name;

  Module() = default;

  explicit Module(std::string name)
      : name(std::move(name)) {}

  void compile(CodeGenerator& W) const {
    compile_tl_h_file(W);
    if (!is_empty()) {
      compile_tl_cpp_file(W);
    }
  }

  void compile_tl_h_file(CodeGenerator& W) const;

  void compile_tl_cpp_file(CodeGenerator& W) const;

  void add_obj(const std::unique_ptr<vk::tlo_parsing::combinator>& f);

  void add_obj(const std::unique_ptr<vk::tlo_parsing::type>& t);

  static std::string get_module_name(const std::string& type_or_comb_name) {
    auto pos = type_or_comb_name.find('.');
    return pos == std::string::npos ? "common" : type_or_comb_name.substr(0, pos);
  }

  static void add_to_module(const std::unique_ptr<vk::tlo_parsing::type>& t) {
    ensure_existence(get_module_name(t->name)).add_obj(t);
  }

  static void add_to_module(const std::unique_ptr<vk::tlo_parsing::combinator>& c) {
    ensure_existence(get_module_name(c->name)).add_obj(c);
  }

  bool is_empty() const;

private:
  static Module& ensure_existence(const std::string& module_name) {
    if (modules.find(module_name) == modules.end()) {
      modules[module_name] = Module(module_name);
    }
    return modules[module_name];
  }

  void update_dependencies(const std::unique_ptr<vk::tlo_parsing::combinator>& combinator);

  void update_dependencies(const std::unique_ptr<vk::tlo_parsing::type>& t);

  void collect_deps_from_type_tree(vk::tlo_parsing::expr_base* expr);
};

} // namespace tl2cpp
