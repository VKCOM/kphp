// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "compiler/const-manipulations.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/pipes/sync.h"
#include "compiler/vertex.h"

template<class DataT>
class DataStream;

class CalcRealDefinesAndAssignModulitesF final : public SyncPipeF<FunctionPtr> {
private:
  using Base = SyncPipeF<FunctionPtr>;
  std::set<std::string*> in_progress;
  std::vector<std::string*> stack;

  CheckConstWithDefines check_const;
  CheckConstAccess check_const_access;
  MakeConst make_const;

  void process_define_recursive(VertexPtr root);
  void process_define(DefinePtr def);

  void print_error_infinite_define(DefinePtr cur_def);

public:
  CalcRealDefinesAndAssignModulitesF();

  void execute(FunctionPtr f, DataStream<FunctionPtr>& unused_os) override;
  void on_finish(DataStream<FunctionPtr>& os) final;
};
