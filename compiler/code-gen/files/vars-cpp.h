#pragma once

#include <vector>

#include "compiler/code-gen/code-generator.h"

struct VarsCpp {
  VarsCpp(std::vector<VarPtr> &&vars, size_t parts_cnt);
  void compile(CodeGenerator &W) const;

private:
  std::vector<VarPtr> vars_;
  size_t parts_cnt_;
};
