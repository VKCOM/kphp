#include "compiler/pipes/calc-actual-edges.h"

#include "compiler/function-pass.h"

class CalcActualCallsEdgesPass : public FunctionPassBase {
  vector<FunctionAndEdges::EdgeInfo> *edges = new vector<FunctionAndEdges::EdgeInfo>();   // деструктора нет намеренно
  int inside_try = 0;

public:
  string get_description() {
    return "Collect actual calls edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__ ((unused))) {
    if (v->type() == op_func_call || v->type() == op_constructor_call || v->type() == op_func_ptr) {
      edges->emplace_back(v->get_func_id(), inside_try);
    }
    if (v->type() == op_throw && !inside_try) {
      current_function->root->throws_flag = true;
    }
    return v;
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (v->type() == op_try) {
      VertexAdaptor<op_try> try_v = v.as<op_try>();
      inside_try++;
      visit(try_v->try_cmd());
      inside_try--;
      visit(try_v->catch_cmd());
      return true;
    }
    return false;
  }

  vector<FunctionAndEdges::EdgeInfo> *get_edges() {
    return edges;
  }
};


void CalcActualCallsEdgesF::execute(FunctionPtr function, DataStream<FunctionAndEdges> &os) {
  AUTO_PROF (calc_actual_calls_edges);
  CalcActualCallsEdgesPass pass;
  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  os << FunctionAndEdges(function, pass.get_edges());
}
