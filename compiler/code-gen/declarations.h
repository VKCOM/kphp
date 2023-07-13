// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/tlo-parsing/tl-objects.h"

#include "compiler/code-gen/code-gen-root-cmd.h"
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
  explicit TlDependentTypesUsings(vk::tlo_parsing::type *tl_type, const std::string &php_tl_class_name);

  void compile(CodeGenerator &W) const;
  void compile_dependencies(CodeGenerator &W);
private:
  struct InnerParamTypeAccess {
    bool drop_class_instance{false};
    std::string inner_type_name;

    InnerParamTypeAccess() = default;

    InnerParamTypeAccess(bool drop_class_instance, const std::string &inner_type_name) :
        drop_class_instance(drop_class_instance),
        inner_type_name(inner_type_name) {}
  };

  struct DeducingInfo {
    std::string deduced_type;
    std::vector<InnerParamTypeAccess> path_to_inner_param;

    DeducingInfo() = default;
    DeducingInfo(std::string deduced_type, std::vector<TlDependentTypesUsings::InnerParamTypeAccess> path);
  };

  vk::tlo_parsing::type *tl_type;
  std::map<std::string, DeducingInfo> deduced_params;
  IncludesCollector dependencies;
  std::string specialization_suffix;
  // Deducing context
  vk::tlo_parsing::combinator *cur_tl_constructor{};
  vk::tlo_parsing::arg *cur_tl_arg{};

  void deduce_params_from_type_tree(vk::tlo_parsing::type_expr_base *type_tree, std::vector<InnerParamTypeAccess> &recursion_stack);
  bool check_deduction_result() const;
};

struct FFIDeclaration : CodeGenRootCmd {
  ClassPtr ffi_scope;
  explicit FFIDeclaration(ClassPtr ffi_scope);
  void compile(CodeGenerator &W) const final;
};

struct InterfaceDeclaration : CodeGenRootCmd {
  InterfacePtr interface;
  explicit InterfaceDeclaration(InterfacePtr interface);
  void compile(CodeGenerator &W) const final;
private:
  std::unique_ptr<TlDependentTypesUsings> detect_if_needs_tl_usings() const;
};

struct ClassDeclaration : CodeGenRootCmd {
  ClassPtr klass;
  explicit ClassDeclaration(ClassPtr klass);
  void compile(CodeGenerator &W) const final;
  static void compile_inner_methods(CodeGenerator &W, ClassPtr klass);

private:
  static void compile_fields(CodeGenerator &W, ClassPtr klass);
  static void compile_json_flatten_flag(CodeGenerator &W, ClassPtr klass);
  static void compile_has_wakeup_flag(CodeGenerator &W, ClassPtr klass);
  static void compile_get_class(CodeGenerator &W, ClassPtr klass);
  static void compile_get_hash(CodeGenerator &W, ClassPtr klass);
  static void compile_accept_visitor_methods(CodeGenerator &W, ClassPtr klass);
  static void compile_msgpack_declarations(CodeGenerator &W, ClassPtr klass);
  static void compile_virtual_builtin_functions(CodeGenerator &W, ClassPtr klass);
  static void compile_wakeup(CodeGenerator &W, ClassPtr klass);

  template<class ReturnValueT>
  static void compile_class_method(FunctionSignatureGenerator &&W, ClassPtr klass, vk::string_view method_signature, const ReturnValueT &return_value);

  static void compile_accept_visitor(CodeGenerator &W, ClassPtr klass, const char *visitor_type);
  static void compile_generic_accept(CodeGenerator &W, ClassPtr klass);
  static void compile_accept_json_visitor(CodeGenerator &W, ClassPtr klass);
  IncludesCollector compile_front_includes(CodeGenerator &W) const;
  void compile_back_includes(CodeGenerator &W, IncludesCollector &&front_includes) const;
  void compile_job_worker_shared_memory_piece_methods(CodeGenerator &W, bool compile_declaration_only = false) const;
  void declare_all_variables(VertexPtr v, CodeGenerator &W) const;
  std::unique_ptr<TlDependentTypesUsings> detect_if_needs_tl_usings() const;
};

struct ClassMembersDefinition : CodeGenRootCmd {
  ClassPtr klass;
  explicit ClassMembersDefinition(ClassPtr klass)
    : klass(klass) {}
  void compile(CodeGenerator &W) const final;

private:
  static void compile_generic_accept(CodeGenerator &W, ClassPtr klass);
  static void compile_generic_accept_instantiations(CodeGenerator &W, ClassPtr klass, vk::string_view type);
  static void compile_accept_json_visitor(CodeGenerator &W, ClassPtr klass);
  static void compile_msgpack_serialize(CodeGenerator &W, ClassPtr klass);
  static void compile_msgpack_deserialize(CodeGenerator &W, ClassPtr klass);
};

struct StaticLibraryRunGlobal {
  gen_out_style style;
  explicit StaticLibraryRunGlobal(gen_out_style style);

  void compile(CodeGenerator &W) const;
};
