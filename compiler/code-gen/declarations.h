#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/gen-out-style.h"
#include "compiler/code-gen/naming.h"
#include "compiler/compiler-core.h"
#include "compiler/data/data_ptr.h"
#include "compiler/vertex-meta_op_base.h"
#include "compiler/code-gen/includes.h"

struct VarDeclaration {
  VarPtr var;
  bool extern_flag;
  bool defval_flag;
  VarDeclaration(VarPtr var, bool extern_flag = false, bool defval_flag = true);
  void compile(CodeGenerator &W) const;
};

VarDeclaration VarExternDeclaration(VarPtr var);
VarDeclaration VarPlainDeclaration(VarPtr var);

struct FunctionDeclaration {
  FunctionPtr function;
  bool in_header;
  gen_out_style style;
  FunctionDeclaration(FunctionPtr function, bool in_header = false, gen_out_style style = gen_out_style::cpp);
  void compile(CodeGenerator &W) const;
};

struct FunctionForkDeclaration {
  FunctionPtr function;
  bool in_header;
  FunctionForkDeclaration(FunctionPtr function, bool in_header = false);
  void compile(CodeGenerator &W) const;
};

struct FunctionParams {
  FunctionPtr function;
  VertexRange params;
  bool in_header;
  gen_out_style style;
  size_t shift;

  FunctionParams(FunctionPtr function, bool in_header = false, gen_out_style style = gen_out_style::cpp);

  FunctionParams(FunctionPtr function, size_t shift, bool in_header = false, gen_out_style style = gen_out_style::cpp);

  void compile(CodeGenerator &W) const;

private:
  void declare_cpp_param(CodeGenerator &W, VertexAdaptor<op_var> var, const TypeName &type) const;
  void declare_txt_param(CodeGenerator &W, VertexAdaptor<op_var> var, const TypeName &type) const;
};

struct TlDependentTypesUsings {
  TlDependentTypesUsings() = default;
  explicit TlDependentTypesUsings(vk::tl::type *tl_type, const std::string &php_tl_class_name);

  void compile(CodeGenerator &W) const;
  void compile_dependencies(CodeGenerator &W);
private:
  struct InnerParamTypeAccess {
    bool drop_class_instance{false};
    std::string inner_type_name;

    InnerParamTypeAccess() = default;
  };

  struct DeducingInfo {
    std::string deduced_type;
    std::vector<InnerParamTypeAccess> path_to_inner_param;

    DeducingInfo() = default;
    DeducingInfo(std::string deduced_type, vector<TlDependentTypesUsings::InnerParamTypeAccess> path);
  };

  std::map<std::string, DeducingInfo> deduced_params;
  IncludesCollector dependencies;
  std::string specialization_suffix;
  // Deducing context
  vk::tl::combinator *cur_tl_constructor{};
  vk::tl::arg *cur_tl_arg{};

  void deduce_params_from_type_tree(vk::tl::type_expr_base *type_tree, std::vector<InnerParamTypeAccess> &recursion_stack);
};

struct InterfaceDeclaration {
  InterfacePtr interface;
  explicit InterfaceDeclaration(InterfacePtr interface);
  void compile(CodeGenerator &W) const;
private:
  std::unique_ptr<TlDependentTypesUsings> detect_if_needs_tl_usings() const;
};

struct ClassDeclaration {
  ClassPtr klass;
  explicit ClassDeclaration(ClassPtr klass);
  void compile(CodeGenerator &W) const;
  static void compile_inner_methods(CodeGenerator &W, ClassPtr klass);
  static void compile_get_class(CodeGenerator &W, ClassPtr klass);
  static void compile_get_hash(CodeGenerator &W, ClassPtr klass);
  static void compile_accept_visitor_methods(CodeGenerator &W, ClassPtr klass);
private:
  template<class ReturnValueT>
  static void compile_class_method(CodeGenerator &W, ClassPtr klass, vk::string_view method_signature, const ReturnValueT &return_value);

  static void compile_accept_visitor(CodeGenerator &W, ClassPtr klass, const char *visitor_type);
  IncludesCollector compile_front_includes(CodeGenerator &W) const;
  void compile_back_includes(CodeGenerator &W, IncludesCollector &&front_includes) const;
  void declare_all_variables(VertexPtr v, CodeGenerator &W) const;
  std::unique_ptr<TlDependentTypesUsings> detect_if_needs_tl_usings() const;
};

struct StaticLibraryRunGlobal {
  gen_out_style style;
  explicit StaticLibraryRunGlobal(gen_out_style style);

  void compile(CodeGenerator &W) const;
};
