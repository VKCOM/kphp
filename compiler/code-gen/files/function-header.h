#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct FunctionH {
  FunctionPtr function;
  FunctionH(FunctionPtr function);
  void compile(CodeGenerator &W) const;
};

