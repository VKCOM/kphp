#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/gen-out-style.h"
#include "compiler/code-gen/naming.h"
#include "compiler/compiler-core.h"
#include "compiler/data/data_ptr.h"
#include "compiler/vertex-meta_op_base.h"

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

struct InterfaceDeclaration {
  InterfacePtr interface;
  explicit InterfaceDeclaration(InterfacePtr interface);
  void compile(CodeGenerator &W) const;
};

struct ClassDeclaration {
  ClassPtr klass;
  explicit ClassDeclaration(ClassPtr klass);
  void compile(CodeGenerator &W) const;
  static void compile_get_class(CodeGenerator &W, ClassPtr klass);
  static void compile_accept_visitor_methods(CodeGenerator &W, ClassPtr klass);
private:
  static void compile_accept_visitor(CodeGenerator &W, ClassPtr klass, const char *visitor);
  void compile_includes(CodeGenerator &W) const;
  void declare_all_variables(VertexPtr v, CodeGenerator &W) const;
};

struct ClassForwardDeclaration {
  ClassPtr klass;
  explicit ClassForwardDeclaration(ClassPtr klass);
  void compile(CodeGenerator &W) const;
};

struct StaticLibraryRunGlobal {
  gen_out_style style;
  explicit StaticLibraryRunGlobal(gen_out_style style);

  void compile(CodeGenerator &W) const;
};
