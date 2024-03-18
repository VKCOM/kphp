// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

// Two passes calculate whether a function can throw or not: CalcActualCallsEdgesPass and FilterOnlyActuallyUsedFunctionsF.
//
// CalcActualCallsEdgesPass records exceptions that were thrown from the function directly (via throw statement).
// It also records all calls that may throw an exception. At that point we don't know whether they can throw
// or not, so we collect them for the next pass (FilterOnlyActuallyUsedFunctionsF).
//
// FilterOnlyActuallyUsedFunctionsF marks functions that call throwing functions as can_throw (unless these exceptions are caught).
//
// op_try vertices start with catches_all=true. When reaching a throw (direct or indirect via call) that is
// not handled by the surrounding try, we mark it with catches_all=false.
//
// Note: we could mark unreachable catch clauses with some kind of flag and remove them during CFG,
// this would result in a better generated C++ code for questionable code.
// We could also report that some catch clauses are unreachable with that info,
// but NoVerify linter already reports those (and some other PHP linters do so as well).
// In case we will ever want to collect that info:
// 1. Change exception_is_handled() to find_catch() so it returns the matched op_catch
// 2. In find_catch() call sites mark the returned catch as reachable
// In CFG, when finding first unreachable catch, break from the loop and keep them disconnected from the graph.
// Note that `try { no throw here } catch (Exception $e) {}` case would have 0 reachable catch clauses
// and we can replace op_try with it's cmd() op_seq.

class CalcActualCallsEdgesPass final : public FunctionPassBase {
public:
  struct EdgeInfo {
    FunctionPtr called_f;
    std::vector<VertexAdaptor<op_try>> try_stack; // exceptions caught at this call site
    bool inside_fork;
    bool can_throw = true; // always `true` for now

    EdgeInfo(const FunctionPtr &called_f, std::vector<VertexAdaptor<op_try>> try_stack, bool inside_fork)
      : called_f(called_f)
      , try_stack(std::move(try_stack))
      , inside_fork(inside_fork) {}
  };

private:
  std::vector<EdgeInfo> edges;
  std::vector<VertexAdaptor<op_try>> try_stack_;
  int inside_fork = 0;

public:
  static bool handle_exception(std::vector<VertexAdaptor<op_try>> &try_stack, ClassPtr thrown_class);

  std::string get_description() override {
    return "Collect actual calls edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;

  bool user_recursion(VertexPtr v) override;

  std::vector<EdgeInfo> get_data() {
    return std::move(edges);
  }
};
