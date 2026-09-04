// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct GlobalVarsMemoryStats : CodeGenRootCmd {
  explicit GlobalVarsMemoryStats(const std::vector<VarPtr>& all_globals);

  void compile(CodeGenerator& W) const final;

private:
  static void compile_getter_part(CodeGenerator& W, int part_id, const std::vector<VarPtr>& global_vars, int offset, int count);

  std::vector<VarPtr> all_nonprimitive_globals;

  static constexpr const char* getter_name_ = "globals_memory_stats_impl"; // hardcoded in runtime, see f$get_global_vars_memory_stats()
  static constexpr int N_GLOBALS_PER_FILE = 512;
};
