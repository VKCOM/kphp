// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/type-data.h"

struct ExternInclude {
  explicit ExternInclude(vk::string_view file_name);
  void compile(CodeGenerator &W) const;

protected:
  vk::string_view file_name;
};

struct Include : private ExternInclude {
  using ExternInclude::ExternInclude;
  void compile(CodeGenerator &W) const;
};

struct LibInclude : private ExternInclude {
  LibInclude(vk::string_view absolute_path, vk::string_view relative_path) noexcept
    : ExternInclude(relative_path)
    , absolute_path(absolute_path) {}
  void compile(CodeGenerator &W) const;
  vk::string_view absolute_path;
};

struct IncludesCollector {
public:
  void add_function_body_depends(const FunctionPtr &function);
  void add_function_signature_depends(const FunctionPtr &function);
  void add_var_signature_depends(const VarPtr &var);
  void add_vertex_depends(VertexPtr v);


  void add_class_forward_declaration(const ClassPtr &klass);
  void add_var_signature_forward_declarations(const VarPtr &var);

  void add_class_include(const ClassPtr &klass);
  void add_base_classes_include(const ClassPtr &klass);
  void add_all_class_types(const TypeData &tinf_type);

  void add_raw_filename_include(const std::string &file_name);

  void compile(CodeGenerator &W) const;

  void start_next_block();

private:
  std::unordered_set<ClassPtr> classes_;
  std::set<std::string> internal_headers_;
  std::map<std::string, std::string> lib_headers_;
  std::set<ClassPtr> forward_declarations_;

  std::unordered_set<ClassPtr> prev_classes_forward_declared_;
  std::unordered_set<ClassPtr> prev_classes_;
  std::set<std::string> prev_headers_;
};
