// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
struct TlTemplatePhpTypeHelpers {
  explicit TlTemplatePhpTypeHelpers(const vk::tlo_parsing::type* type)
      : type(get_this_from_renamed_tl_scheme(type)),
        constructor(nullptr) {}

  explicit TlTemplatePhpTypeHelpers(const vk::tlo_parsing::combinator* constructor)
      : type(nullptr),
        constructor(get_this_from_renamed_tl_scheme(constructor)) {}

  void compile(CodeGenerator& W) const {
    size_t cnt = 0;
    std::vector<std::string> type_var_names;
    const vk::tlo_parsing::combinator* target_constructor = type ? type->constructors[0].get() : constructor;
    for (const auto& arg : target_constructor->args) {
      if (arg->is_type()) {
        ++cnt;
        type_var_names.emplace_back(arg->name);
      }
    }
    const std::string& struct_name = cpp_tl_struct_name("", type ? type->name : constructor->name, "__");
    W << "template <" << vk::join(std::vector<std::string>(cnt, "typename"), ", ") << ">" << NL;
    W << "struct " << struct_name << " " << BEGIN << "using type = tl_undefined_php_type;" << NL << END << ";" << NL << NL;
    auto php_classes = type ? get_all_php_classes_of_tl_type(type) : get_all_php_classes_of_tl_constructor(constructor);
    for (const auto& cur_php_class_template_instantiation : php_classes) {
      const std::string& cur_instantiation_name = cur_php_class_template_instantiation->src_name;
      std::vector<std::string> template_specialization_params = type_var_names;
      std::string type_var_access;
      if (type) {
        type_var_access = cur_instantiation_name;
      } else {
        kphp_assert(constructor);
        if (tl->get_type_by_magic(constructor->type_id)->name == "ReqResult") {
          type_var_access = "C$VK$TL$RpcResponse";
        } else {
          size_t pos = cur_instantiation_name.find("__");
          kphp_assert(pos != std::string::npos);
          ClassPtr tl_type_php_class = get_php_class_of_tl_type_specialization(tl->get_type_by_magic(constructor->type_id), cur_instantiation_name.substr(pos));
          kphp_assert(tl_type_php_class);
          type_var_access = tl_type_php_class->src_name;
        }
      }
      std::transform(template_specialization_params.begin(), template_specialization_params.end(), template_specialization_params.begin(),
                     [&](const std::string& s) { return type_var_access + "::" += s; });
      W << "template <>" << NL;
      W << "struct " << struct_name << "<" << vk::join(template_specialization_params, ", ") << "> " << BEGIN;
      W << "using type = " << cur_instantiation_name << ";" << NL;
      W << END << ";" << NL << NL;
    }
  }

private:
  const vk::tlo_parsing::type* type;
  const vk::tlo_parsing::combinator* constructor;
};
} // namespace tl2cpp
