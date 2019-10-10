#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct GlobalVarsMemoryStats {
  explicit GlobalVarsMemoryStats(const std::vector<SrcFilePtr> &main_files);

  void compile(CodeGenerator &W) const;

private:
  void compile_getter_part(CodeGenerator &W, const std::set<VarPtr> &global_vars, size_t part_id) const;

  const std::string getter_name_{"get_global_vars_memory_stats_impl"};
  std::vector<SrcFilePtr> main_files_;
};
