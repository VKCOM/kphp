// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

struct GlobalVarsReset : CodeGenRootCmd {
  explicit GlobalVarsReset(SrcFilePtr main_file);

  void compile(CodeGenerator &W) const final;

  static void compile_part(FunctionPtr func, const std::set<VarPtr> &used_vars, int part_i, CodeGenerator &W);

  static void compile_func(FunctionPtr func, int parts_n, CodeGenerator &W);

  static void declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W);

private:
  SrcFilePtr main_file_;
};
