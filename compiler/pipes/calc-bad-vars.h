#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

struct DepData {
  vector<FunctionPtr> dep;
  vector<VarPtr> used_global_vars;

  vector<VarPtr> used_ref_vars;
  vector<std::pair<VarPtr, VarPtr>> ref_ref_edges;
  vector<std::pair<VarPtr, VarPtr>> global_ref_edges;

  vector<FunctionPtr> forks;
};


class CalcBadVarsF {
private:
  DataStream<std::pair<FunctionPtr, DepData>> tmp_stream;
public:

  CalcBadVarsF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionPtr function, DataStream<FunctionPtr> &);


  void on_finish(DataStream<FunctionPtr> &os);
};
