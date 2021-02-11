#pragma once

#include "compiler/code-gen/code-generator.h"

struct CodeGenRootCmd {
  virtual ~CodeGenRootCmd() = default;
  virtual void compile(CodeGenerator &W) const = 0;
};

