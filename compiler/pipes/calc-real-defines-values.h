// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/const-manipulations.h"
#include "compiler/pipes/sync.h"
#include "compiler/vertex.h"

class CalcRealDefinesValuesF final : public SyncPipeF<FunctionPtr> {
private:
  using Base = SyncPipeF<FunctionPtr>;
  std::set<string*> in_progress;
  std::vector<string*> stack;

  CheckConstWithDefines check_const;
  CheckConstAccess check_const_access;
  MakeConst make_const;

  void process_define_recursive(SrcFilePtr file, VertexPtr root);
  void process_define(DefinePtr def);

  void print_error_infinite_define(DefinePtr cur_def);

public:

  CalcRealDefinesValuesF();

  void on_finish(DataStream<FunctionPtr> &os) final;
};
