#pragma once

#include "compiler/function-pass.h"

class CalcValRefPass : public FunctionPassBase {
public:
  string get_description() {
    return "Calc val ref";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  bool is_allowed_for_getting_val_or_ref(Operation op, bool is_last);

  struct LocalT : public FunctionPassBase::LocalT {
    bool allowed;
  };

  void on_enter_edge(VertexPtr vertex, LocalT *local, VertexPtr dest_vertex, LocalT *dest_local);

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local, VisitT &visit) {
    for (VertexPtr &child : *v) {
      local->allowed = is_allowed_for_getting_val_or_ref(v->type(), &child == &v->back());
      visit(child);
    }
    return true;
  }
};
