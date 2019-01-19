#pragma once

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

class CalcActualCallsEdgesPass : public FunctionPassBase {
public:
  struct EdgeInfo {
    FunctionPtr called_f;
    bool inside_try;

    EdgeInfo(const FunctionPtr &called_f, bool inside_try) :
      called_f(called_f),
      inside_try(inside_try) {}
  };

private:
  std::vector<EdgeInfo> edges;
  int inside_try = 0;

public:
  string get_description() {
    return "Collect actual calls edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__ ((unused)));

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

  std::vector<EdgeInfo> on_finish() {
    return std::move(edges);
  }
};


