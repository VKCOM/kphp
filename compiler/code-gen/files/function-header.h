// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct FunctionH : CodeGenRootCmd {
  FunctionPtr function;
  explicit FunctionH(FunctionPtr function);
  void compile(CodeGenerator &W) const final;
};

