// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

struct GlobalVarsReset : CodeGenRootCmd {
  explicit GlobalVarsReset(std::vector<VarPtr> &&all_globals);

  void compile(CodeGenerator &W) const final;

  static void compile_globals_reset_part(const std::vector<VarPtr> &used_vars, int part_id, CodeGenerator &W);
  static void compile_globals_reset(int parts_n, CodeGenerator &W);

private:
  std::vector<VarPtr> all_globals;
};
