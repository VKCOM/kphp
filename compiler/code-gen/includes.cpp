// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/includes.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"

ExternInclude::ExternInclude(vk::string_view file_name) :
  file_name(file_name) {
  kphp_assert(!file_name.empty());
}

void ExternInclude::compile(CodeGenerator &W) const {
  W << "#include \"" << file_name << "\"" << NL;
}

void Include::compile(CodeGenerator &W) const {
  W.add_include(static_cast<std::string>(file_name));
  ExternInclude::compile(W);
}

void LibInclude::compile(CodeGenerator &W) const {
  W.add_lib_include(static_cast<std::string>(file_name));
  ExternInclude::compile(W);
}

void IncludesCollector::add_function_body_depends(const FunctionPtr &function) {
  for (auto to_include : function->dep) {
    if (to_include == function) {
      continue;
    }
    if (to_include->is_imported_from_static_lib()) {
      lib_headers_.emplace(to_include->header_full_name);
    } else if (!to_include->is_extern()) {
      kphp_assert(!to_include->header_full_name.empty());
      internal_headers_.emplace(to_include->header_full_name);
    }
  }

  for (auto to_include : function->class_dep) {
    add_class_include(to_include);
  }

  for (auto local_var : function->local_var_ids) {
    add_var_signature_depends(local_var);
  }

  for (auto global_var : function->global_var_ids) {
    add_var_signature_depends(global_var);
  }

  if (function->tl_common_h_dep) {    // functions that use a typed TL RPC need to see t_ReqResult during the compilation
    kphp_error(!G->settings().tl_schema_file.get().empty(), "tl schema not given as -T option for compilation");
    internal_headers_.emplace("tl/common.h");
  }
}

void IncludesCollector::add_function_signature_depends(const FunctionPtr &function) {
  add_all_class_types(*function->tinf_node.get_type());
  for (const auto &param : function->param_ids) {
    add_all_class_types(*param->tinf_node.get_type());
  }
}

void IncludesCollector::add_class_include(const ClassPtr &klass) {
  classes_.emplace(klass);
}

void IncludesCollector::add_class_forward_declaration(const ClassPtr &klass) {
  forward_declarations_.emplace(klass);
}

void IncludesCollector::add_var_signature_forward_declarations(const VarPtr &var) {
  std::unordered_set<ClassPtr> all_classes;
  var->tinf_node.get_type()->get_all_class_types_inside(all_classes);
  for (auto klass: all_classes) {
    add_class_forward_declaration(klass);
  }
}

void IncludesCollector::add_base_classes_include(const ClassPtr &klass) {
  classes_.insert(klass->implements.cbegin(), klass->implements.cend());

  if (ClassData::does_need_codegen(klass->parent_class)) {
    classes_.emplace(klass->parent_class);
  }
}

void IncludesCollector::add_var_signature_depends(const VarPtr &var) {
  add_all_class_types(*var->tinf_node.get_type());
}

void IncludesCollector::add_all_class_types(const TypeData &tinf_type) {
  if (tinf_type.has_class_type_inside()) {
    tinf_type.get_all_class_types_inside(classes_);
  }
}

void IncludesCollector::add_raw_filename_include(const std::string &file_name) {
  internal_headers_.emplace(file_name);
}

void IncludesCollector::compile(CodeGenerator &W) const {
  for (const auto &lib_header : lib_headers_) {
    if (!prev_headers_.count(lib_header)) {
      W << LibInclude(lib_header);
    }
  }

  std::set<std::string> class_headers;
  for (const auto &klass : classes_) {
    if (!prev_classes_.count(klass) && !klass->is_builtin()) {
      class_headers.emplace(klass->get_subdir() + "/" + klass->header_name);
    }
  }
  for (const auto &class_header : class_headers) {
    W << Include(class_header);
  }

  std::set<vk::string_view> class_forward_declarations;
  for (const auto &klass : forward_declarations_) {
    if (!prev_classes_forward_declared_.count(klass) &&
        !prev_classes_.count(klass) &&
        !classes_.count(klass)) {
      class_forward_declarations.emplace(klass->src_name);
    }
  }

  if (!class_forward_declarations.empty()) {
    W << OpenNamespace();
    for (const auto &class_name : class_forward_declarations) {
      W << "struct " << class_name << ";" << NL;
    }
    W << CloseNamespace();
  }

  for (const auto &internal_header : internal_headers_) {
    if (!prev_headers_.count(internal_header)) {
      W << Include(internal_header);
    }
  }
}

void IncludesCollector::start_next_block() {
  prev_classes_forward_declared_.insert(forward_declarations_.cbegin(), forward_declarations_.cend());
  forward_declarations_.clear();

  prev_classes_.insert(classes_.cbegin(), classes_.cend());
  classes_.clear();

  prev_headers_.insert(internal_headers_.begin(), internal_headers_.end());
  prev_headers_.insert(lib_headers_.begin(), lib_headers_.end());
  internal_headers_.clear();
  lib_headers_.clear();
}
