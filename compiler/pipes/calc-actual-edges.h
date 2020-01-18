#pragma once

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

class CalcActualCallsEdgesPass : public FunctionPassBase {
public:
  struct EdgeInfo {
    FunctionPtr called_f;
    bool inside_try;
    bool inside_fork;

    EdgeInfo(const FunctionPtr &called_f, bool inside_try, bool inside_fork) :
      called_f(called_f),
      inside_try(inside_try),
      inside_fork(inside_fork)
      {}
  };

private:
  std::vector<EdgeInfo> edges;
  int inside_try = 0;
  int inside_fork = 0;

public:
  string get_description() {
    return "Collect actual calls edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__ ((unused)));

  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitVertex<CalcActualCallsEdgesPass> &visit);

  std::vector<EdgeInfo> on_finish() {
    return std::move(edges);
  }
};


