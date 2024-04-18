// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct GlobalVarsReset : CodeGenRootCmd {
  explicit GlobalVarsReset(std::vector<std::vector<VarPtr>> &&all_globals_batched);

  void compile(CodeGenerator &W) const final;

  static void compile_globals_reset_part(CodeGenerator &W, int batch_num, const std::vector<VarPtr> &cur_batch);
  static void compile_globals_reset(CodeGenerator &W, int parts_cnt);
  static void compile_globals_allocate(CodeGenerator &W);

private:
  std::vector<std::vector<VarPtr>> all_globals_batched;
};
