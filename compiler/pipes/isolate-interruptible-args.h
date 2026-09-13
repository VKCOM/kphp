// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// K2 only.
//
// `CO_AWAIT_TASK_ON_STACK(F(args...))` requests a stack allocation for the task<T> that F(args...)
// is about to create, before args... are evaluated (see runtime-light/coroutine/task.h). If evaluating
// args... happens to construct some other, unrelated task<T> that isn't consumed synchronously (e.g. a
// nested fork(...) or an RPC send that hands a task to the scheduler without co_await-ing it), that stray
// allocation steals the pending request and the real one crashes an assertion in task_allocator.
//
// This pass hoists every call nested (at any depth) inside the argument subtree of an interruptible call
// that will not itself compile as a safely CO_AWAIT_TASK_ON_STACK-wrapped call, into a temp-var
// assignment placed before the enclosing statement. Nested interruptible calls are left in place: their
// own request/consume cycle resolves synchronously and is already safe.
//
// while/do/for loop conditions are re-evaluated on every iteration, so hoisting out of them the usual
// way (once, before the loop) would only run the hoisted call on the first iteration. Whenever such a
// condition contains an unsafe call reachable from an active interruptible-call window, the loop itself
// is restructured into an unconditional loop with an equivalent `if (...) break;` at the top of its body,
// so the condition is processed like any other statement on every iteration instead of being hoisted.
class IsolateInterruptibleArgsPass final : public FunctionPassBase {
private:
  static std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> make_temp_var(VertexPtr init) noexcept;
  // Builds `if (should_break_cond) { break; }`.
  static VertexAdaptor<op_if> make_break_if(VertexPtr should_break_cond) noexcept;
  // Creates a fresh local bool variable used to distinguish a do-loop's first iteration (see process_do).
  static VertexAdaptor<op_var> make_loop_guard_var(VertexPtr location_source) noexcept;

  static VertexPtr process(VertexPtr vertex, bool in_interruptible_call, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;
  static VertexPtr process_block(VertexAdaptor<op_seq> block) noexcept;
  static VertexPtr process_fork(VertexAdaptor<op_fork> fork_call, bool in_interruptible_call, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;
  static VertexPtr process_func_call(VertexAdaptor<op_func_call> call, bool in_interruptible_call, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;
  static VertexPtr process_while(VertexAdaptor<op_while> while_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;
  static VertexPtr process_do(VertexAdaptor<op_do> do_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;
  static VertexPtr process_for(VertexAdaptor<op_for> for_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept;

  // Read-only counterpart of process_fork/process_func_call's hoisting decision: true if `vertex`
  // contains a fork(...)/non-interruptible call that would need hoisting were it reachable from an
  // active interruptible-call window. Used to decide whether a loop condition needs restructuring.
  static bool contains_unsafe_call_in_window(VertexPtr vertex, bool in_interruptible_call) noexcept;

public:
  std::string get_description() override {
    return "Isolate interruptible call arguments";
  }

  bool check_function(FunctionPtr function) const override;

  bool user_recursion(VertexPtr vertex) override;

  VertexPtr on_enter_vertex(VertexPtr vertex) override;
};
