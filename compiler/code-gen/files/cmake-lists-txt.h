// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"

class CodeGenerator;

struct CmakeListsTxt : CodeGenRootCmd {
  void compile(CodeGenerator &W) const noexcept final;

private:
  void compile_include_cmake() const noexcept;
  void compile_cmake_lists_txt(CodeGenerator &W) const noexcept;
};
