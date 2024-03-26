// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct ConstVarsInit : CodeGenRootCmd {
  ConstVarsInit(std::vector<VarPtr> &&all_constants, int count_per_part);
  
  void compile(CodeGenerator &W) const final;

  static void compile_const_init_part(CodeGenerator &W, int part_id, const std::vector<VarPtr> &all_constants, int offset, int count);
  static void compile_const_init(CodeGenerator &W, int parts_cnt, const std::vector<int> &max_dep_levels);

private:
  std::vector<VarPtr> all_constants;
  const int count_per_part;
};
