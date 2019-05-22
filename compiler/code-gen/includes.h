#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/inferring/type-data.h"

struct ExternInclude {
  explicit ExternInclude(const vk::string_view &file_name);
  void compile(CodeGenerator &W) const;

protected:
  vk::string_view file_name;
};

struct Include : private ExternInclude {
  using ExternInclude::ExternInclude;
  void compile(CodeGenerator &W) const;
};

struct LibInclude : private ExternInclude {
  using ExternInclude::ExternInclude;
  void compile(CodeGenerator &W) const;
};

struct IncludesCollector {
public:
  void add_function_body_depends(const FunctionPtr &function);
  void add_function_signature_depends(const FunctionPtr &function);
  void add_var_signature_depends(const VarPtr &var);

  void add_class_include(const ClassPtr &klass);
  void add_implements_include(const ClassPtr &klass);
  void add_all_class_types(const TypeData &tinf_type);

  void compile(CodeGenerator &W) const;

  void start_next_block();

private:

  std::unordered_set<ClassPtr> classes_;
  std::set<std::string> internal_headers_;
  std::set<std::string> lib_headers_;

  std::unordered_set<ClassPtr> prev_classes_;
  std::set<std::string> prev_headers_;
};
