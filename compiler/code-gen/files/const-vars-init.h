// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/const-globals-linear-mem.h"

struct ConstVarsInit : CodeGenRootCmd {
  explicit ConstVarsInit(const ConstantsLinearMem &all_constants_in_mem);
  
  void compile(CodeGenerator &W) const final;

  static void compile_const_init_part(CodeGenerator& W, const ConstantsLinearMem::OneBatchInfo& dir_batch);
  static void compile_const_init(CodeGenerator &W, const ConstantsLinearMem &all_constants_in_mem);

private:
  const ConstantsLinearMem &all_constants_in_mem;
};
