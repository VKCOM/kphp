#pragma once


#include "compiler/inferring/public.h"
#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class CommonAnalyzerPass : public FunctionPassBase {
  void check_set(VertexAdaptor<op_set> to_check);
  void analyzer_check_array(VertexPtr to_check);

public:

  string get_description() {
    return "Try to detect common errors";
  }

  bool check_function(FunctionPtr function) {
    if (!default_check_function(function) || function->is_extern()) {
      return false;
    }

    for (VarPtr &var : function->local_var_ids) {
      G->stats.cnt_mixed_vars += tinf::get_type(var)->ptype() == tp_var;
    }

    for (VarPtr &var : function->param_ids) {
      G->stats.cnt_mixed_params += tinf::get_type(var)->ptype() == tp_var;
      G->stats.cnt_const_mixed_params += (tinf::get_type(var)->ptype() == tp_var) && var->is_read_only;
    }

    return true;
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;

    LocalT() :
      from_seq() {}
  };

  void on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr , LocalT *dest_local);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT * local);
};
