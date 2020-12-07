// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

class CalcActualCallsEdgesPass final : public FunctionPassBase {
public:
  enum class TryKind: uint8_t {
    // CatchesNone - no try/catch wraps this edge (exceptions will escape)
    CatchesNone,

    // CatchesSome - there is a try/catch block, but it may not catch all possible exceptions
    CatchesSome,

    // CatchesAll - there is a try/catch block that is exhaustive and will not let any exceptions escape
    CatchesAll,
  };

  struct EdgeInfo {
    FunctionPtr called_f;
    TryKind try_kind;
    bool inside_fork;

    EdgeInfo(const FunctionPtr &called_f, TryKind try_kind, bool inside_fork) :
      called_f(called_f),
      try_kind(try_kind),
      inside_fork(inside_fork)
      {}
  };

private:
  std::vector<EdgeInfo> edges;
  TryKind try_kind = TryKind::CatchesNone;
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


