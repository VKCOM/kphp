// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"

struct GlobalVarsCppPart : CodeGenRootCmd {
  GlobalVarsCppPart(std::vector<VarPtr> &&vars_of_part, size_t part_id);
  void compile(CodeGenerator &W) const final;

private:
  mutable std::vector<VarPtr> vars_of_part_;
  size_t part_id;
};
