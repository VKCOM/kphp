// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct StaticLibraryRunGlobalHeaderH : CodeGenRootCmd {
  void compile(CodeGenerator &W) const final;
};

struct LibHeaderH : CodeGenRootCmd {
  FunctionPtr exported_function;
  explicit LibHeaderH(FunctionPtr exported_function);
  void compile(CodeGenerator &W) const final;
};

struct LibHeaderTxt : CodeGenRootCmd {
  std::vector<FunctionPtr> exported_functions;
  explicit LibHeaderTxt(std::vector<FunctionPtr> &&exported_functions);
  void compile(CodeGenerator &W) const final;
};
