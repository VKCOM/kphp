#pragma once

#include "compiler/function-pass.h"

/*** Calculate const_type for all nodes ***/
class CalcConstTypePass : public FunctionPassBase {
private:
  AUTO_PROF (calc_const_type);
public:
  struct LocalT : public FunctionPassBase::LocalT {
    bool has_nonconst;

    LocalT() :
      has_nonconst(false) {
    }
  };

  string get_description() {
    return "Calc const types";
  }

  void on_exit_edge(VertexPtr, LocalT *v_local, VertexPtr, LocalT *);

  VertexPtr on_exit_vertex(VertexPtr v, LocalT *local);
};
