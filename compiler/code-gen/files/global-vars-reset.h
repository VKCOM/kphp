// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct GlobalVarsReset : CodeGenRootCmd {
  explicit GlobalVarsReset(std::vector<VarPtr> &&all_globals, int count_per_part);

  void compile(CodeGenerator &W) const final;

  static void compile_globals_reset_part(CodeGenerator &W, int part_id, const std::vector<VarPtr> &all_globals, int offset, int count);
  static void compile_globals_reset(CodeGenerator &W, int parts_cnt);
  static void compile_globals_allocate(CodeGenerator &W);

private:
  std::vector<VarPtr> all_globals;
  const int count_per_part;
};
