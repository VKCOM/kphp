// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/const-globals-batched-mem.h"

struct GlobalVarsReset : CodeGenRootCmd {
  explicit GlobalVarsReset(const GlobalsBatchedMem &all_globals_in_mem);

  void compile(CodeGenerator &W) const final;

  static void compile_globals_reset_part(CodeGenerator &W, const GlobalsBatchedMem::OneBatchInfo &batch);
  static void compile_globals_reset(CodeGenerator &W, const GlobalsBatchedMem &all_globals_in_mem);
  static void compile_globals_allocate(CodeGenerator &W);

private:
  const GlobalsBatchedMem &all_globals_in_mem;
};
