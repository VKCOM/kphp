// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct VarsCpp : CodeGenRootCmd {
  VarsCpp(std::vector<VarPtr> &&vars, size_t parts_cnt);
  void compile(CodeGenerator &W) const;

private:
  std::vector<VarPtr> vars_;
  size_t parts_cnt_;
};
