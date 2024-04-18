// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct ConstVarsInit : CodeGenRootCmd {
  explicit ConstVarsInit(std::vector<std::vector<VarPtr>> &&all_constants_batched);
  
  void compile(CodeGenerator &W) const final;

  static void compile_const_init_part(CodeGenerator &W, int batch_num, const std::vector<VarPtr> &cur_batch);
  static void compile_const_init(CodeGenerator &W, int n_batches, const std::vector<int> &max_dep_levels);

private:
  std::vector<std::vector<VarPtr>> all_constants_batched;
};
