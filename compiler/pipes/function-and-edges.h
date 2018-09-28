#pragma once

#include <vector>

#include "compiler/data.h"
#include "compiler/data_ptr.h"

struct FunctionAndEdges {
  struct EdgeInfo {
    FunctionPtr called_f;
    bool inside_try;

    EdgeInfo(const FunctionPtr &called_f, bool inside_try) :
      called_f(called_f),
      inside_try(inside_try) {}
  };

  FunctionPtr function;
  vector<EdgeInfo> *edges;

  FunctionAndEdges() :
    function(),
    edges(nullptr) {
  }

  FunctionAndEdges(FunctionPtr function, vector<EdgeInfo> *edges) :
    function(function),
    edges(edges) {
  }
};

/*
static bool operator<(const FunctionAndEdges &a, const FunctionAndEdges &b) {
  return a.function->id < b.function->id;
}
*/
