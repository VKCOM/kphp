// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// K2 only.
//
// CO_AWAIT_TASK_ON_STACK(F(args...)) requests a stack allocation for the task<T> that F(args...)
// is about to create, before args... are evaluated (see runtime-light/coroutine/task.h). If evaluating
// args... happens to construct some other task<T>, it crashes an assertion in task_allocator. This pass hoists every
// call nested (at any depth) inside the argument subtree of an interruptible call
// into a temp var assignment placed before the enclosing statement, so its body always runs with no
// pending request.
class IsolateInterruptibleArgsPass final : public FunctionPassBase {
private:
  static VertexAdaptor<op_var> declare_temp_var(VertexPtr type_source) noexcept;
  static std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> make_temp_var(VertexPtr init) noexcept;
  // Builds if (should_break_cond) { break; }.
  static VertexAdaptor<op_if> make_break_if(VertexPtr should_break_cond) noexcept;
  // Creates a fresh local bool variable used to distinguish a do-loop's first iteration (see process_do).
  static VertexAdaptor<op_var> make_loop_guard_var(VertexPtr location_source) noexcept;

  static VertexPtr process(VertexPtr vertex, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_block(VertexAdaptor<op_seq> block) noexcept;
  static VertexPtr process_fork(VertexAdaptor<op_fork> fork_call, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_func_call(VertexAdaptor<op_func_call> call, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_null_coalesce(VertexAdaptor<op_null_coalesce> null_coalesce, bool in_interruptible_call,
                                         std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_ternary(VertexAdaptor<op_ternary> ternary, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_lazy_logical_op(VertexAdaptor<meta_op_binary> op, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_while(VertexAdaptor<op_while> while_v, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_do(VertexAdaptor<op_do> do_v, std::vector<VertexPtr>& pending_hoists) noexcept;
  static VertexPtr process_for(VertexAdaptor<op_for> for_v, std::vector<VertexPtr>& pending_hoists) noexcept;

  static bool contains_unsafe_call_in_window(VertexPtr vertex, bool in_interruptible_call) noexcept;

public:
  std::string get_description() override {
    return "Isolate interruptible call arguments";
  }

  bool check_function(FunctionPtr function) const override;

  bool user_recursion(VertexPtr vertex) override;

  VertexPtr on_enter_vertex(VertexPtr vertex) override;
};
