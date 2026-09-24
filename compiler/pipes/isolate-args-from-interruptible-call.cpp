// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/isolate-args-from-interruptible-call.h"

#include <algorithm>

#include "auto/compiler/vertex/vertex-types.h"
#include "common/algorithms/find.h"
#include "compiler/compiler-core.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/public.h"
#include "compiler/kphp_assert.h"
#include "compiler/name-gen.h"
#include "compiler/vertex-util.h"

auto IsolateArgsFromInterruptibleCallPass::declare_temp_var(VertexPtr type_source) noexcept -> VertexAdaptor<op_var> {
  auto temp_var = VertexAdaptor<op_var>::create().set_location(type_source);
  temp_var->str_val = gen_unique_name("isolated_arg");
  temp_var->var_id = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  temp_var->var_id->tinf_node.copy_type_from(tinf::get_type(type_source));

  return temp_var;
}

auto IsolateArgsFromInterruptibleCallPass::make_temp_var(VertexPtr init) noexcept -> std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> {
  auto temp_var = declare_temp_var(init);
  auto set_op = VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), init).set_rl_type(val_none).set_location(init);
  temp_var->val_ref_flag = init->val_ref_flag;
  auto move_op = VertexAdaptor<op_move>::create(temp_var.set_rl_type(val_r)).set_rl_type(val_r).set_location(init);

  return {move_op, set_op};
}

auto IsolateArgsFromInterruptibleCallPass::make_break_if(VertexPtr should_break_cond) noexcept -> VertexAdaptor<op_if> {
  auto break_v = VertexAdaptor<op_break>::create(VertexUtil::create_int_const(1)).set_location(should_break_cond);
  return VertexAdaptor<op_if>::create(should_break_cond, VertexUtil::embrace(break_v)).set_rl_type(val_none).set_location(should_break_cond);
}

auto IsolateArgsFromInterruptibleCallPass::make_loop_guard_var(VertexPtr location_source) noexcept -> VertexAdaptor<op_var> {
  auto guard_var = VertexAdaptor<op_var>::create().set_location(location_source);
  guard_var->str_val = gen_unique_name("loop_first_iter_guard");
  guard_var->var_id = G->create_local_var(stage::get_function(), guard_var->str_val, VarData::var_local_t);
  guard_var->var_id->tinf_node.copy_type_from(tinf::get_type(VertexAdaptor<op_true>::create().set_location(location_source)));

  return guard_var;
}

auto IsolateArgsFromInterruptibleCallPass::needs_hoist(VertexPtr vertex, bool in_interruptible_call) noexcept -> bool {
  if (auto fork_call = vertex.try_as<op_fork>()) {
    auto call = fork_call->func_call();
    for (VertexPtr arg : call->args()) {
      if (needs_hoist(arg, false)) {
        return true;
      }
    }

    return in_interruptible_call;
  }

  if (auto call = vertex.try_as<op_func_call>()) {
    bool is_interruptible_call = call->func_id->is_interruptible;
    for (VertexPtr arg : call->args()) {
      if (needs_hoist(arg, is_interruptible_call)) {
        return true;
      }
    }

    return in_interruptible_call;
  }

  if (auto null_coalesce = vertex.try_as<op_null_coalesce>()) {
    bool own_window = VertexUtil::is_interruptible_expr(null_coalesce->rhs());
    if (needs_hoist(null_coalesce->lhs(), own_window)) {
      return true;
    }

    return in_interruptible_call;
  }

  return std::any_of(vertex->begin(), vertex->end(), [in_interruptible_call](VertexPtr vertex) noexcept { return needs_hoist(vertex, in_interruptible_call); });
}

auto IsolateArgsFromInterruptibleCallPass::process_fork(VertexAdaptor<op_fork> fork_call, bool in_interruptible_call,
                                                        std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  auto call = fork_call->func_call();
  for (VertexPtr& arg : call->args()) {
    arg = process(arg, false, pending_hoists);
  }

  if (!in_interruptible_call) {
    return fork_call;
  }

  auto temp_var = make_temp_var(fork_call);
  pending_hoists.emplace_back(temp_var.second);

  return temp_var.first;
}

auto IsolateArgsFromInterruptibleCallPass::process_func_call(VertexAdaptor<op_func_call> call, bool in_interruptible_call,
                                                             std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  bool is_interruptible_call = call->func_id->is_interruptible;
  for (VertexPtr& arg : call->args()) {
    arg = process(arg, is_interruptible_call, pending_hoists);
  }

  if (!in_interruptible_call) {
    return call;
  }

  if (tinf::get_type(call)->ptype() == tp_void) {
    pending_hoists.emplace_back(call);
    return VertexAdaptor<op_null>::create().set_location(call);
  }

  auto temp_var = make_temp_var(call);
  pending_hoists.emplace_back(temp_var.second);

  return temp_var.first;
}

auto IsolateArgsFromInterruptibleCallPass::process_null_coalesce(VertexAdaptor<op_null_coalesce> null_coalesce, bool in_interruptible_call,
                                                                 std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  // See compile_null_coalesce: the whole expression is wrapped in its own CO_AWAIT_TASK_ON_STACK only when
  // rhs itself contains an interruptible call. In that case, lhs is evaluated while constructing
  // NullCoalesce<T>(lhs), which happens strictly inside this node's own freshly-opened window - regardless
  // of whether an ambient one is also active.
  bool own_window = VertexUtil::is_interruptible_expr(null_coalesce->rhs());
  null_coalesce->lhs() = process(null_coalesce->lhs(), own_window, pending_hoists);

  // rhs, when non-trivial, is compiled into a lambda invoked lazily from inside NullCoalesce<T>::finalize().
  // By the time that invocation happens, all windows have already been consumed - so rhs never runs inside an opened window.
  // Hoists from rhs must be local in generated lambda.
  VertexPtr original_rhs = null_coalesce->rhs();
  std::vector<VertexPtr> rhs_hoists;
  auto rhs_expr = process(original_rhs, false, rhs_hoists);
  if (!rhs_hoists.empty()) {
    rhs_hoists.emplace_back(rhs_expr);
    auto wrapped = VertexAdaptor<op_seq_rval>::create(rhs_hoists).set_location(null_coalesce);
    wrapped->tinf_node.copy_type_from(tinf::get_type(original_rhs));
    wrapped->throw_flag = original_rhs->throw_flag;
    null_coalesce->rhs() = wrapped;
  } else {
    null_coalesce->rhs() = rhs_expr;
  }

  if (!in_interruptible_call) {
    return null_coalesce;
  }

  auto temp_var = make_temp_var(null_coalesce);
  pending_hoists.emplace_back(temp_var.second);

  return temp_var.first;
}

auto IsolateArgsFromInterruptibleCallPass::process_ternary(VertexAdaptor<op_ternary> ternary, bool in_interruptible_call,
                                                           std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  ternary->cond() = process(ternary->cond(), in_interruptible_call, pending_hoists);

  bool true_needs_hoist = needs_hoist(ternary->true_expr(), in_interruptible_call);
  bool false_needs_hoist = needs_hoist(ternary->false_expr(), in_interruptible_call);
  if (!true_needs_hoist && !false_needs_hoist) {
    ternary->true_expr() = process(ternary->true_expr(), in_interruptible_call, pending_hoists);
    ternary->false_expr() = process(ternary->false_expr(), in_interruptible_call, pending_hoists);
    return ternary;
  }

  // At least one branch contains a call that would need hoisting under the ambient window, but only the
  // taken branch may actually run at runtime - unconditionally hoisting it (like a plain call would) would
  // run it regardless of cond. Instead the ternary is rewritten into if (cond) { tmp = true_expr; } else
  // { tmp = false_expr; }, so each branch's own hoists stay local to that branch's op_seq instead of
  // escaping to before the whole statement.
  bool is_void = tinf::get_type(ternary)->ptype() == tp_void;

  // Each branch now runs inside its own if/else arm, which is itself hoisted before the call that opened
  // the window - so by the time this code runs, no window is active anymore. Recursing with the ambient
  // in_interruptible_call here would make a call inside the branch get hoisted a second, redundant time.
  std::vector<VertexPtr> true_hoists;
  auto true_expr = process(ternary->true_expr(), false, true_hoists);
  std::vector<VertexPtr> false_hoists;
  auto false_expr = process(ternary->false_expr(), false, false_hoists);

  if (is_void) {
    true_hoists.emplace_back(true_expr);
    false_hoists.emplace_back(false_expr);
    auto true_branch = VertexAdaptor<op_seq>::create(true_hoists).set_rl_type(val_none).set_location(ternary);
    auto false_branch = VertexAdaptor<op_seq>::create(false_hoists).set_rl_type(val_none).set_location(ternary);

    return VertexAdaptor<op_if>::create(ternary->cond(), true_branch, false_branch).set_rl_type(val_none).set_location(ternary);
  }

  auto temp_var = declare_temp_var(ternary);

  true_hoists.emplace_back(VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), true_expr).set_rl_type(val_none).set_location(ternary));
  auto true_branch = VertexAdaptor<op_seq>::create(true_hoists).set_rl_type(val_none).set_location(ternary);

  false_hoists.emplace_back(VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), false_expr).set_rl_type(val_none).set_location(ternary));
  auto false_branch = VertexAdaptor<op_seq>::create(false_hoists).set_rl_type(val_none).set_location(ternary);

  auto if_v = VertexAdaptor<op_if>::create(ternary->cond(), true_branch, false_branch).set_rl_type(val_none).set_location(ternary);
  pending_hoists.emplace_back(if_v);

  temp_var->val_ref_flag = ternary->val_ref_flag;

  return VertexAdaptor<op_move>::create(temp_var.set_rl_type(val_r)).set_rl_type(val_r).set_location(ternary);
}

auto IsolateArgsFromInterruptibleCallPass::process_lazy_logical_op(VertexAdaptor<meta_op_binary> op, bool in_interruptible_call,
                                                                   std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  // op_log_and/op_log_or/_let compile to plain C++ && / || (compile_binary_op) - always short-circuiting,
  // never wrapped in a window of their own. lhs is unconditionally evaluated - safe to process in place. rhs
  // is conditional; if it needs a hoist under an active ambient window, hoisting it unconditionally would run it regardless of lhs, so the node
  // is rewritten into an equivalent if/else assigning a temp var instead.
  op->lhs() = process(op->lhs(), in_interruptible_call, pending_hoists);

  if (!needs_hoist(op->rhs(), in_interruptible_call)) {
    op->rhs() = process(op->rhs(), in_interruptible_call, pending_hoists);
    return op;
  }

  bool is_and = vk::any_of_equal(op->type(), op_log_and, op_log_and_let);

  auto temp_var = declare_temp_var(op);

  std::vector<VertexPtr> rhs_hoists;
  // rhs now runs inside its own if/else arm, itself hoisted before the call that opened the window - so a
  // call inside it does not need to be hoisted again.
  auto rhs_expr = process(op->rhs(), false, rhs_hoists);
  rhs_hoists.emplace_back(VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), rhs_expr).set_rl_type(val_none).set_location(op));
  auto rhs_branch = VertexAdaptor<op_seq>::create(rhs_hoists).set_rl_type(val_none).set_location(op);

  VertexPtr short_circuit_value =
      is_and ? VertexPtr(VertexAdaptor<op_false>::create().set_location(op)) : VertexPtr(VertexAdaptor<op_true>::create().set_location(op));
  auto short_circuit_set = VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), short_circuit_value).set_rl_type(val_none).set_location(op);
  auto short_circuit_branch = VertexAdaptor<op_seq>::create(short_circuit_set).set_rl_type(val_none).set_location(op);

  auto if_v = is_and ? VertexAdaptor<op_if>::create(op->lhs(), rhs_branch, short_circuit_branch)
                     : VertexAdaptor<op_if>::create(op->lhs(), short_circuit_branch, rhs_branch);
  if_v.set_rl_type(val_none).set_location(op);
  pending_hoists.emplace_back(if_v);

  temp_var->val_ref_flag = op->val_ref_flag;

  return VertexAdaptor<op_move>::create(temp_var.set_rl_type(val_r)).set_rl_type(val_r).set_location(op);
}

auto IsolateArgsFromInterruptibleCallPass::process_while(VertexAdaptor<op_while> while_v, std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  if (needs_hoist(while_v->cond(), false)) {
    auto should_break = VertexAdaptor<op_log_not>::create(while_v->cond()).set_location(while_v);
    auto break_if = make_break_if(should_break);
    while_v->cmd_ref() = VertexAdaptor<op_seq>::create(break_if, while_v->cmd()).set_rl_type(val_none).set_location(while_v);
    while_v->cond() = VertexAdaptor<op_true>::create().set_location(while_v);
  } else {
    std::vector<VertexPtr> hoists;
    while_v->cond() = process(while_v->cond(), false, hoists);
    kphp_assert(hoists.empty());
  }

  while_v->cmd_ref() = process(while_v->cmd_ref(), false, pending_hoists);

  return while_v;
}

auto IsolateArgsFromInterruptibleCallPass::process_do(VertexAdaptor<op_do> do_v, std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  if (needs_hoist(do_v->cond(), false)) {
    // A first-iteration guard variable: on the first iteration the && short
    // circuits before cond is ever touched; on later iterations it's already false and cond decides.
    auto first_iter_var = make_loop_guard_var(do_v);

    auto init_guard = VertexAdaptor<op_set>::create(first_iter_var.clone().set_rl_type(val_l), VertexAdaptor<op_true>::create().set_location(do_v))
                          .set_rl_type(val_none)
                          .set_location(do_v);
    pending_hoists.emplace_back(init_guard);

    auto not_first_iter = VertexAdaptor<op_log_not>::create(first_iter_var.clone().set_rl_type(val_r)).set_location(do_v);
    auto cond_is_false = VertexAdaptor<op_log_not>::create(do_v->cond()).set_location(do_v);
    auto should_break = VertexAdaptor<op_log_and>::create(not_first_iter, cond_is_false).set_location(do_v);
    auto break_if = make_break_if(should_break);

    auto clear_guard = VertexAdaptor<op_set>::create(first_iter_var.clone().set_rl_type(val_l), VertexAdaptor<op_false>::create().set_location(do_v))
                           .set_rl_type(val_none)
                           .set_location(do_v);

    do_v->cmd_ref() = VertexAdaptor<op_seq>::create(break_if, clear_guard, do_v->cmd()).set_rl_type(val_none).set_location(do_v);
    do_v->cond() = VertexAdaptor<op_true>::create().set_location(do_v);
  } else {
    std::vector<VertexPtr> hoists;
    do_v->cond() = process(do_v->cond(), false, hoists);
    kphp_assert(hoists.empty());
  }

  do_v->cmd_ref() = process(do_v->cmd_ref(), false, pending_hoists);

  return do_v;
}

auto IsolateArgsFromInterruptibleCallPass::process_for(VertexAdaptor<op_for> for_v, std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  if (needs_hoist(for_v->cond(), false)) {
    // cond is always an op_seq_comma: every expression but the last runs purely for its side effects, and the last one
    // (already bool-converted by the parser) is the actual condition. Move the non-last expressions to the
    // top of the body as plain statements (preserving their once-per-iteration evaluation and order), and
    // turn the last one into an equivalent if (!last) break; cond itself becomes an always-true stub.
    std::vector<VertexPtr> cond_children;
    for (VertexPtr child : *for_v->cond()) {
      cond_children.emplace_back(child);
    }

    std::vector<VertexPtr> new_body_children;
    for (size_t i = 0; i + 1 < cond_children.size(); ++i) {
      new_body_children.emplace_back(cond_children[i].set_rl_type(val_none));
    }

    auto should_break = VertexAdaptor<op_log_not>::create(cond_children.back()).set_location(for_v);
    new_body_children.emplace_back(make_break_if(should_break));
    new_body_children.emplace_back(for_v->cmd());

    for_v->cmd_ref() = VertexAdaptor<op_seq>::create(new_body_children).set_rl_type(val_none).set_location(for_v);
    for_v->cond() = VertexAdaptor<op_seq_comma>::create(VertexAdaptor<op_true>::create().set_location(for_v)).set_location(for_v);
  } else {
    std::vector<VertexPtr> hoists;
    for_v->cond() = process(for_v->cond(), false, hoists);
    kphp_assert(hoists.empty());
  }

  // pre_cond and post_cond are themselves op_seq (self-scoping via process_block below), so unlike cond
  // they're safe to walk into: pre_cond runs once, post_cond's hoists stay inside its own per-iteration
  // scope instead of escaping to before the loop.
  for_v->pre_cond_ref() = process(for_v->pre_cond_ref(), false, pending_hoists);
  for_v->post_cond_ref() = process(for_v->post_cond_ref(), false, pending_hoists);
  for_v->cmd_ref() = process(for_v->cmd_ref(), false, pending_hoists);

  return for_v;
}

auto IsolateArgsFromInterruptibleCallPass::process_block(VertexAdaptor<op_seq> block) noexcept -> VertexPtr {
  for (VertexPtr& statement : *block) {
    std::vector<VertexPtr> hoists;
    statement = process(statement, false, hoists);
    if (!hoists.empty()) {
      statement = VertexAdaptor<op_seq>::create(hoists, statement).set_rl_type(val_none).set_location(statement);
    }
  }

  return block;
}

auto IsolateArgsFromInterruptibleCallPass::process(VertexPtr vertex, bool in_interruptible_call, std::vector<VertexPtr>& pending_hoists) noexcept -> VertexPtr {
  switch (vertex->type()) {
  case op_seq:
    return process_block(vertex.as<op_seq>());
  case op_fork:
    return process_fork(vertex.as<op_fork>(), in_interruptible_call, pending_hoists);
  case op_func_call:
    return process_func_call(vertex.as<op_func_call>(), in_interruptible_call, pending_hoists);
  case op_null_coalesce:
    return process_null_coalesce(vertex.as<op_null_coalesce>(), in_interruptible_call, pending_hoists);
  case op_ternary:
    return process_ternary(vertex.as<op_ternary>(), in_interruptible_call, pending_hoists);
  case op_log_and:
  case op_log_or:
  case op_log_and_let:
  case op_log_or_let:
    return process_lazy_logical_op(vertex.as<meta_op_binary>(), in_interruptible_call, pending_hoists);
  // while/do/for loop-controlling conditions are re-evaluated on every iteration: hoisting anything
  // out of them into the statement that precedes the whole loop would only run it once. Instead, when a
  // condition contains an unsafe call, the loop is rewritten into an unconditional loop that performs the
  // equivalent check (and breaks) at the top of its body, on every iteration - see process_while/_do/_for.
  case op_while:
    return process_while(vertex.as<op_while>(), pending_hoists);
  case op_do:
    return process_do(vertex.as<op_do>(), pending_hoists);
  case op_for:
    return process_for(vertex.as<op_for>(), pending_hoists);
  default:
    std::for_each(vertex->begin(), vertex->end(),
                  [in_interruptible_call, &pending_hoists](VertexPtr& vertex) noexcept { vertex = process(vertex, in_interruptible_call, pending_hoists); });
  }

  return vertex;
}

std::string IsolateArgsFromInterruptibleCallPass::get_description() {
  return "Isolate interruptible call arguments";
}

bool IsolateArgsFromInterruptibleCallPass::check_function(FunctionPtr function) const {
  return G->is_output_mode_k2() && !function->is_extern();
}

bool IsolateArgsFromInterruptibleCallPass::user_recursion(VertexPtr /*unused*/) {
  return true;
}

VertexPtr IsolateArgsFromInterruptibleCallPass::on_enter_vertex(VertexPtr vertex) {
  auto function_v = vertex.as<op_function>();
  function_v->cmd_ref() = process_block(function_v->cmd());

  return vertex;
}
