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
