// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-generator.h"

struct StaticLibraryRunGlobalHeaderH {
  void compile(CodeGenerator &W) const;
};

struct LibHeaderH {
  FunctionPtr exported_function;
  LibHeaderH(FunctionPtr exported_function);
  void compile(CodeGenerator &W) const;
};

struct LibHeaderTxt {
  std::vector<FunctionPtr> exported_functions;
  LibHeaderTxt(std::vector<FunctionPtr> &&exported_functions);
  void compile(CodeGenerator &W) const;
};
