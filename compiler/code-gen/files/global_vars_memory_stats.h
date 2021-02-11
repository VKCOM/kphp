// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct GlobalVarsMemoryStats : CodeGenRootCmd {
  explicit GlobalVarsMemoryStats(SrcFilePtr main_file);

  void compile(CodeGenerator &W) const final;

private:
  void compile_getter_part(CodeGenerator &W, const std::set<VarPtr> &global_vars, size_t part_id) const;

  const std::string getter_name_{"get_global_vars_memory_stats_impl"};
  SrcFilePtr main_file_;
};
