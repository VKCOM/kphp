// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct GlobalVarsDeclarations : CodeGenRootCmd {
  GlobalVarsDeclarations(std::vector<VarPtr> &&all_mutable_globals);
  
  void compile(CodeGenerator &W) const final;

private:
  std::vector<VarPtr> all_mutable_globals;
};
