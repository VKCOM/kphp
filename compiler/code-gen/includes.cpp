#include "compiler/code-gen/includes.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/pipes/code-gen/common-code-gen.h"

ExternInclude::ExternInclude(const vk::string_view &file_name) :
  file_name(file_name) {
}

void ExternInclude::compile(CodeGenerator &W) const {
  W << "#include \"" << file_name << "\"" << NL;
}

void Include::compile(CodeGenerator &W) const {
  W.get_writer().add_include(static_cast<std::string>(file_name));
  ExternInclude::compile(W);
}

void LibInclude::compile(CodeGenerator &W) const {
  W.get_writer().add_lib_include(static_cast<std::string>(file_name));
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
}

void IncludesCollector::add_function_signature_depends(const FunctionPtr &function) {
  for (const auto &tinf_node : function->tinf_nodes) {
    add_all_class_types(*tinf_node.get_type());
  }
}

void IncludesCollector::add_class_include(const ClassPtr &klass) {
  classes_.emplace(klass);
}

void IncludesCollector::add_implements_include(const ClassPtr &klass) {
  if (!klass->implements.empty()) {
    for (auto &interface : klass->implements) {
      classes_.emplace(interface);
    }
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

void IncludesCollector::compile(CodeGenerator &W) const {
  for (auto lib_header : lib_headers_) {
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

  for (const auto &internal_header : internal_headers_) {
    if (!prev_headers_.count(internal_header)) {
      W << Include(internal_header);
    }
  }
}

void IncludesCollector::start_next_block() {
  prev_classes_.insert(classes_.cbegin(), classes_.cend());
  classes_.clear();

  prev_headers_.insert(internal_headers_.begin(), internal_headers_.end());
  prev_headers_.insert(lib_headers_.begin(), lib_headers_.end());
  internal_headers_.clear();
  lib_headers_.clear();
}
