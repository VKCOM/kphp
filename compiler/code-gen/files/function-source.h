#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct FunctionCpp {
  FunctionPtr function;
  FunctionCpp(FunctionPtr function);
  void compile(CodeGenerator &W) const;
};

void declare_global_vars(FunctionPtr function, CodeGenerator &W);
void declare_const_vars(FunctionPtr function, CodeGenerator &W);
void declare_static_vars(FunctionPtr function, CodeGenerator &W);
