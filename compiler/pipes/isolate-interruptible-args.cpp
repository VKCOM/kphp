// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/isolate-interruptible-args.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/vertex-util.h"

std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> IsolateInterruptibleArgsPass::make_temp_var(VertexPtr init) noexcept {
  auto temp_var = VertexAdaptor<op_var>::create().set_location(init);
  temp_var->str_val = gen_unique_name("isolated_interruptible_arg");
  temp_var->var_id = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
  temp_var->var_id->tinf_node.copy_type_from(tinf::get_type(init));
  auto set_op = VertexAdaptor<op_set>::create(temp_var.clone().set_rl_type(val_l), init).set_rl_type(val_none).set_location(init);
  temp_var->val_ref_flag = init->val_ref_flag;
  auto move_op = VertexAdaptor<op_move>::create(temp_var.set_rl_type(val_r)).set_rl_type(val_r).set_location(init);

  return {move_op, set_op};
}

VertexPtr IsolateInterruptibleArgsPass::process_fork(VertexAdaptor<op_fork> fork_call, bool in_interruptible_call,
                                                     std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  // 'fork(...)' is always compiled via 'func_call_mode::fork_call', which never wraps the call in
  // CO_AWAIT_TASK_ON_STACK, regardless of the wrapped function's own is_interruptible flag. So its args open a fresh window, and the
  // whole fork(...) expression itself is an "unsafe" (non-wrapped) task-producing construct.
  auto call = fork_call->func_call();
  for (VertexPtr& arg : call->args()) {
    arg = process(arg, false, pending_hoists);
  }

  if (!in_interruptible_call) {
    return fork_call;
  }

  auto temp_var = make_temp_var(fork_call);
  pending_hoists.push_back(temp_var.second);

  return temp_var.first;
}

VertexPtr IsolateInterruptibleArgsPass::process_func_call(VertexAdaptor<op_func_call> call, bool in_interruptible_call,
                                                          std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  // Any op_func_call reached through normal recursion (i.e. not as op_fork's direct wrapped call,
  // handled separately in process_fork) always compiles in simple/async_call mode: if it's
  // interruptible, it will be wrapped in its own CO_AWAIT_TASK_ON_STACK, and its own request/consume
  // cycle resolves synchronously - safe. Otherwise it's an "unsafe" construct just like fork(...).
  bool is_interruptible_call = call->func_id->is_interruptible;
  for (VertexPtr& arg : call->args()) {
    arg = process(arg, is_interruptible_call, pending_hoists);
  }

  if (is_interruptible_call || !in_interruptible_call) {
    return call;
  }

  auto temp_var = make_temp_var(call);
  pending_hoists.push_back(temp_var.second);

  return temp_var.first;
}

bool IsolateInterruptibleArgsPass::contains_unsafe_call_in_window(VertexPtr vertex, bool in_interruptible_call) noexcept {
  if (auto fork_call = vertex.try_as<op_fork>()) {
    auto call = fork_call->func_call();
    for (VertexPtr arg : call->args()) {
      if (contains_unsafe_call_in_window(arg, false)) {
        return true;
      }
    }
    return in_interruptible_call;
  }
  if (auto call = vertex.try_as<op_func_call>()) {
    bool is_interruptible_call = call->func_id->is_interruptible;
    for (VertexPtr arg : call->args()) {
      if (contains_unsafe_call_in_window(arg, is_interruptible_call)) {
        return true;
      }
    }
    return !is_interruptible_call && in_interruptible_call;
  }

  for (VertexPtr child : *vertex) {
    if (contains_unsafe_call_in_window(child, in_interruptible_call)) {
      return true;
    }
  }

  return false;
}

VertexAdaptor<op_if> IsolateInterruptibleArgsPass::make_break_if(VertexPtr should_break_cond) noexcept {
  auto break_v = VertexAdaptor<op_break>::create(VertexUtil::create_int_const(1)).set_location(should_break_cond);
  return VertexAdaptor<op_if>::create(should_break_cond, VertexUtil::embrace(break_v)).set_rl_type(val_none).set_location(should_break_cond);
}

VertexAdaptor<op_var> IsolateInterruptibleArgsPass::make_loop_guard_var(VertexPtr location_source) noexcept {
  auto guard_var = VertexAdaptor<op_var>::create().set_location(location_source);
  guard_var->str_val = gen_unique_name("interruptible_loop_first_iter");
  guard_var->var_id = G->create_local_var(stage::get_function(), guard_var->str_val, VarData::var_local_t);
  guard_var->var_id->tinf_node.copy_type_from(tinf::get_type(VertexAdaptor<op_true>::create().set_location(location_source)));

  return guard_var;
}

VertexPtr IsolateInterruptibleArgsPass::process(VertexPtr vertex, bool in_interruptible_call, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  if (auto block = vertex.try_as<op_seq>()) {
    return process_block(block);
  }
  if (auto fork_call = vertex.try_as<op_fork>()) {
    return process_fork(fork_call, in_interruptible_call, pending_hoists);
  }
  if (auto call = vertex.try_as<op_func_call>()) {
    return process_func_call(call, in_interruptible_call, pending_hoists);
  }

  // while/do/for loop-controlling conditions are re-evaluated on every iteration: hoisting anything
  // out of them into the statement that precedes the whole loop would only run it once. Instead, when a
  // condition contains an unsafe call, the loop is rewritten into an unconditional loop that performs the
  // equivalent check (and breaks) at the top of its body, on every iteration - see process_while/_do/_for.
  if (auto while_v = vertex.try_as<op_while>()) {
    return process_while(while_v, pending_hoists);
  }
  if (auto do_v = vertex.try_as<op_do>()) {
    return process_do(do_v, pending_hoists);
  }
  if (auto for_v = vertex.try_as<op_for>()) {
    return process_for(for_v, pending_hoists);
  }

  for (VertexPtr& child : *vertex) {
    child = process(child, in_interruptible_call, pending_hoists);
  }

  return vertex;
}

VertexPtr IsolateInterruptibleArgsPass::process_while(VertexAdaptor<op_while> while_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  if (contains_unsafe_call_in_window(while_v->cond(), false)) {
    auto should_break = VertexAdaptor<op_log_not>::create(while_v->cond()).set_location(while_v);
    auto break_if = make_break_if(should_break);
    while_v->cmd_ref() = VertexAdaptor<op_seq>::create(break_if, while_v->cmd()).set_rl_type(val_none).set_location(while_v);
    while_v->cond() = VertexAdaptor<op_true>::create().set_location(while_v);
  }

  while_v->cmd_ref() = process(while_v->cmd_ref(), false, pending_hoists);
  return while_v;
}

VertexPtr IsolateInterruptibleArgsPass::process_do(VertexAdaptor<op_do> do_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  if (contains_unsafe_call_in_window(do_v->cond(), false)) {
    // Neither top-of-body nor bottom-of-body placement alone works for do-while: a native `continue`
    // jumps straight to the `while (cond)` test (bottom placement would be skipped), but `cond` must not
    // be evaluated before the loop's very first iteration (top placement alone would evaluate it too
    // early). A first-iteration guard variable resolves both: on the first iteration the `&&` short
    // circuits before `cond` is ever touched; on later iterations it's already false and `cond` decides.
    auto first_iter_var = make_loop_guard_var(do_v);

    auto init_guard = VertexAdaptor<op_set>::create(first_iter_var.clone().set_rl_type(val_l), VertexAdaptor<op_true>::create().set_location(do_v))
                          .set_rl_type(val_none)
                          .set_location(do_v);
    pending_hoists.push_back(init_guard);

    auto not_first_iter = VertexAdaptor<op_log_not>::create(first_iter_var.clone().set_rl_type(val_r)).set_location(do_v);
    auto cond_is_false = VertexAdaptor<op_log_not>::create(do_v->cond()).set_location(do_v);
    auto should_break = VertexAdaptor<op_log_and>::create(not_first_iter, cond_is_false).set_location(do_v);
    auto break_if = make_break_if(should_break);

    auto clear_guard = VertexAdaptor<op_set>::create(first_iter_var.clone().set_rl_type(val_l), VertexAdaptor<op_false>::create().set_location(do_v))
                           .set_rl_type(val_none)
                           .set_location(do_v);

    do_v->cmd_ref() = VertexAdaptor<op_seq>::create(break_if, clear_guard, do_v->cmd()).set_rl_type(val_none).set_location(do_v);
    do_v->cond() = VertexAdaptor<op_true>::create().set_location(do_v);
  }

  do_v->cmd_ref() = process(do_v->cmd_ref(), false, pending_hoists);
  return do_v;
}

VertexPtr IsolateInterruptibleArgsPass::process_for(VertexAdaptor<op_for> for_v, std::vector<VertexAdaptor<op_set>>& pending_hoists) noexcept {
  if (contains_unsafe_call_in_window(for_v->cond(), false)) {
    // cond is always an op_seq_comma (even for a single expression or the empty/default case, see
    // GenTree::get_for): every expression but the last runs purely for its side effects, and the last one
    // (already bool-converted by the parser) is the actual condition. Move the non-last expressions to the
    // top of the body as plain statements (preserving their once-per-iteration evaluation and order), and
    // turn the last one into an equivalent `if (!last) break;`; cond itself becomes an always-true stub.
    std::vector<VertexPtr> cond_children;
    for (VertexPtr child : *for_v->cond()) {
      cond_children.push_back(child);
    }

    std::vector<VertexPtr> new_body_children;
    for (size_t i = 0; i + 1 < cond_children.size(); ++i) {
      new_body_children.push_back(cond_children[i].set_rl_type(val_none));
    }
    auto should_break = VertexAdaptor<op_log_not>::create(cond_children.back()).set_location(for_v);
    new_body_children.push_back(make_break_if(should_break));
    new_body_children.push_back(for_v->cmd());

    for_v->cmd_ref() = VertexAdaptor<op_seq>::create(new_body_children).set_rl_type(val_none).set_location(for_v);
    for_v->cond() = VertexAdaptor<op_seq_comma>::create(VertexAdaptor<op_true>::create().set_location(for_v)).set_location(for_v);
  }

  // pre_cond and post_cond are themselves op_seq (self-scoping via process_block below), so unlike cond
  // they're safe to walk into: pre_cond runs once, post_cond's hoists stay inside its own per-iteration
  // scope instead of escaping to before the loop.
  for_v->pre_cond_ref() = process(for_v->pre_cond_ref(), false, pending_hoists);
  for_v->post_cond_ref() = process(for_v->post_cond_ref(), false, pending_hoists);
  for_v->cmd_ref() = process(for_v->cmd_ref(), false, pending_hoists);

  return for_v;
}

VertexPtr IsolateInterruptibleArgsPass::process_block(VertexAdaptor<op_seq> block) noexcept {
  for (VertexPtr& statement : *block) {
    std::vector<VertexAdaptor<op_set>> hoists;
    statement = process(statement, false, hoists);
    if (!hoists.empty()) {
      statement = VertexAdaptor<op_seq>::create(hoists, statement).set_rl_type(val_none).set_location(statement);
    }
  }

  return block;
}

bool IsolateInterruptibleArgsPass::user_recursion(VertexPtr /*unused*/) {
  return true;
}

VertexPtr IsolateInterruptibleArgsPass::on_enter_vertex(VertexPtr vertex) {
  auto function_v = vertex.as<op_function>();
  function_v->cmd_ref() = process_block(function_v->cmd());

  return vertex;
}

bool IsolateInterruptibleArgsPass::check_function(FunctionPtr function) const {
  return G->is_output_mode_k2() && !function->is_extern();
}
