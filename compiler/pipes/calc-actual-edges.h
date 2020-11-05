// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

class CalcActualCallsEdgesPass final : public FunctionPassBase {
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
  string get_description() override {
    return "Collect actual calls edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;

  bool user_recursion(VertexPtr v) override;

  std::vector<EdgeInfo> get_data() {
    return std::move(edges);
  }
};


