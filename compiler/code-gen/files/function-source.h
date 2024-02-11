// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct FunctionCpp : CodeGenRootCmd {
  FunctionPtr function;
  explicit FunctionCpp(FunctionPtr function);
  void compile(CodeGenerator &W) const final;
};

struct FunctionListCpp : CodeGenRootCmd {
  std::vector<FunctionPtr> functions;
  explicit FunctionListCpp(std::vector<FunctionPtr> functions) : functions{functions} {}
  void compile(CodeGenerator &W) const final;
};

void declare_vars_vector(const std::vector<VarPtr> &vars, std::unordered_set<VarPtr> *already_declared, CodeGenerator &W);
void declare_vars_set(const std::set<VarPtr> &vars, std::unordered_set<VarPtr> *already_declared, CodeGenerator &W);
