// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-main-edges.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/ifi.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-isset.h"
#include "compiler/inferring/restriction-match-phpdoc.h"
#include "compiler/inferring/restriction-non-void.h"
#include "compiler/inferring/type-node.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"

tinf::Node *CollectMainEdgesPass::node_from_rvalue(const RValue &rvalue) {
  if (rvalue.node == nullptr) {
    kphp_assert (rvalue.type != nullptr);
    return new tinf::TypeNode(rvalue.type, stage::get_location());
  }

  return rvalue.node;
}


// an isset check will analyze expressions that may differ from PHP after getting elements from typed arrays
// for example: if $a is int[], then if($a[100] === 0) may differ from PHP if 100-th element doesn't exist (null vs 0)
void CollectMainEdgesPass::create_isset_check(const RValue &rvalue) {
  tinf::Node *a = node_from_rvalue(rvalue);
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionIsset(a));
}

RValue CollectMainEdgesPass::as_set_value(VertexPtr v) {
  // $lhs = $rhs
  if (v->type() == op_set) {
    return as_rvalue(v.as<op_set>()->rhs());
  }

  // $lhs++ => $lhs = $lhs + 1
  if (vk::any_of_equal(v->type(), op_prefix_inc, op_prefix_dec, op_postfix_dec, op_postfix_inc)) {
    auto unary = v.as<meta_op_unary>();
    auto location = stage::get_location();
    auto one = GenTree::create_int_const(1).set_location(location);
    auto res = VertexAdaptor<op_add>::create(unary->expr(), one).set_location(location);
    return as_rvalue(res);
  }

  // $lhs *= $rhs => $lhs = $lhs * $rhs
  if (auto binary = v.try_as<meta_op_binary>()) {
    VertexPtr res = create_vertex(OpInfo::base_op(v->type()), binary->lhs(), binary->rhs());
    res.set_location(stage::get_location());
    return as_rvalue(res);
  }

  kphp_fail();
  return RValue();
}

// a 'set edge' from lhs to rhs means, that rhs type is copied to lhs type (see VarNode::do_recalc)
// so, lhs becomes at least the same type as rhs
// rhs is an expression (not a type!), so every time rhs is recalculated, lhs is recalculated also
template<class R>
void CollectMainEdgesPass::create_set(const LValue &lhs, const R &rhs) {
  tinf::Edge *edge = new tinf::Edge();
  edge->from = lhs.value;
  edge->from_at = lhs.key;
  edge->to = as_rvalue(rhs).node;   // not node_from_rvalue(), highlighting that TypeNode is unacceptable
  kphp_assert(edge->to);
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);
}

// an 'initial type assign' means, that lhs type is at least initial_type: it's assigned to it once in a given place
// for example, a key in foreach is mixed
// it is very similar to create_set(), but this function exists in parallel to express another semantics
// btw, this edge also appears in stacktrace on type mismatch, answering "why it was inferred in such a way"
void CollectMainEdgesPass::create_type_initial_assign(const LValue &lhs, const TypeData *initial_type) {
  tinf::Edge *edge = new tinf::Edge();
  edge->from = lhs.value;
  edge->from_at = lhs.key;
  edge->to = new tinf::TypeNode(initial_type, stage::get_location());
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);
}

// a 'type assign with restriction' is a phpdoc-controlled type meaning that it is assigned and stays valid during inferring
// for example, function f(?int $x) — then $x is set int|null, and even if passed string to f, $x remains int|null, triggering an error
// if a type contains any, e.g. array $x — then $x is set to any[], this any is inferred via other edges, but passing string triggers an error
// lhs here is VarNode (it means: a variable, or a function param, or a function return)
void CollectMainEdgesPass::create_type_assign_with_restriction(const LValue &var_lhs, const TypeData *assigned_and_expected_type) {
  auto *restricted_node = dynamic_cast<tinf::VarNode *>(var_lhs.value);
  kphp_assert(restricted_node && var_lhs.key->empty());

  // this is just a type assign
  tinf::Edge *edge = new tinf::Edge();
  edge->from = restricted_node;
  edge->from_at = var_lhs.key;
  edge->to = new tinf::TypeNode(assigned_and_expected_type, stage::get_location());
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);

  // this is a restriction, consider VarNode for how it works
  restricted_node->set_type_restriction(assigned_and_expected_type);
}

// a 'postponed type check' is a type check without creating a set node
// for example, function f(?int $x) {} f($v) — $x will remain int|null regardless of $v
// therefore, we don't need to create an edge $x->$v, but after tinf has finished, we need to check that type of $v matches int|null
// this is just an optimization to create less set edges: whenever $v is updated, $x recalculation is not triggered
// without this, everything would still work jue to type checks, but a bit slower due to useless recalculations
void CollectMainEdgesPass::create_postponed_type_check(const RValue &restricted_value, const RValue &actual_value, const TypeData *expected_type) {
  auto *restricted_node = dynamic_cast<tinf::VarNode *>(node_from_rvalue(restricted_value));
  auto *actual_node = node_from_rvalue(actual_value);
  kphp_assert(restricted_node && actual_node);
  tinf::get_inferer()->add_node(actual_node);
  tinf::get_inferer()->add_restriction(new RestrictionMatchPhpdoc(restricted_node, actual_node, expected_type));
}

// a 'non-void check' will test that lhs value is not void
// for example, callback of array_map isn't allowed to return void
void CollectMainEdgesPass::create_non_void(const RValue &lhs) {
  tinf::Node *a = node_from_rvalue(lhs);
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionNonVoid(a));
}


void CollectMainEdgesPass::add_type_rule(VertexAdaptor<op_var> var_op) {
  switch (var_op->type_rule->type()) {
    case op_common_type_rule:
      kphp_assert(0 && "common type rule to be deleted");
      break;
    case op_gt_type_rule:
      kphp_assert(0 && "greater type rule to be deleted");
      break;
    case op_lt_type_rule:
      kphp_assert(0 && "less type rule to be deleted");
      break;
    case op_set_check_type_rule: {
      // this occurs on @var phpdoc for local/static vars inside function (or global vars, for top-level functions)
      const TypeData *var_phpdoc_type = tinf::convert_type_rule_to_TypeData(var_op->type_rule);
      // it may appear duplicated @var for the same variable (strange, but such occasions meet in real code)
      if (var_op->var_id->tinf_node.type_restriction) {
        kphp_error(are_equal_types(var_op->var_id->tinf_node.type_restriction, var_phpdoc_type),
                   fmt_format("Different @var for {} exist", var_op->var_id->get_human_readable_name()));
      } else {
        create_type_assign_with_restriction(as_lvalue(var_op->var_id), var_phpdoc_type);
      }
      break;
    }
    default:
      __builtin_unreachable();
  }
}

// handles calls to extern functions that modify arg type
// for example, array_push($arr, $v) is like $arr[]=$v
void CollectMainEdgesPass::on_func_call_extern_modifying_arg_type(VertexAdaptor<op_func_call> call, FunctionPtr extern_function) {
  if (extern_function->name == "array_unshift" || extern_function->name == "array_push") {
    VertexRange args = call->args();
    LValue val = as_lvalue(args[0]);

    auto key = new MultiKey(*val.key);
    key->push_back(Key::any_key());
    val.key = key;

    for (auto i : VertexRange(args.begin() + 1, args.end())) {
      create_set(val, i);
    }
  }

  if (extern_function->name == "array_merge_into") {
    VertexPtr another_array_arg = call->args()[1];
    LValue dest_array_arg = as_lvalue(call->args()[0]);

    create_set(dest_array_arg, another_array_arg);
  }

  if (extern_function->name == "array_splice" && call->args().size() == 4) {
    VertexPtr another_array_arg = call->args()[3];
    LValue dest_array_arg = as_lvalue(call->args()[0]);

    create_set(dest_array_arg, another_array_arg);
  }


  if (extern_function->name == "wait_queue_push") {
    VertexRange args = call->args();
    LValue future_queue_arg = as_lvalue(args[0]);

    auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
    auto ref = VertexAdaptor<op_type_expr_arg_ref>::create();
    ref->int_val = 2;
    auto rule = GenTree::create_type_help_vertex(tp_future_queue, {VertexAdaptor<op_index>::create(ref)});
    fake_func_call->type_rule = VertexAdaptor<op_common_type_rule>::create(rule);
    fake_func_call->func_id = call->func_id;

    create_set(future_queue_arg, fake_func_call);
  }
}

// handle calls to built-in functions with callbacks:
// array_filter($a, function($v) { ... }), array_filter($a, 'cb') and similar
// (not to php functions with callable arguments! built-in only, having a callback type rule)
void CollectMainEdgesPass::on_func_call_param_callback(VertexAdaptor<op_func_call> call, int param_i, FunctionPtr provided_callback) {
  const FunctionPtr call_function = call->func_id;  // array_filter, etc
  auto callback_param = call_function->get_params()[param_i].as<op_func_param_typed_callback>();

  // set restriction of return type of provided callback
  // built-in functions are declared as 'callback(...) ::: any' or 'callback(...) ::: bool'
  // if 'any', it must return anything but void; otherwise, the type rule must be satisfied (it's static, no argument depends)
  kphp_assert(callback_param->type_rule && callback_param->type_rule->rule()->type() == op_type_expr_type);
  if (callback_param->type_rule->rule()->type_help == tp_Any) {
    create_non_void(as_rvalue(provided_callback, -1));
  } else {
    // todo don't create it every time
    create_postponed_type_check(as_rvalue(provided_callback, -1), as_rvalue(provided_callback, -1), tinf::convert_type_rule_to_TypeData(callback_param->type_rule));
  }

  // extern functions passed as string-callbacks don't need to be handled further
  // (as we assume that they all use cast-params)
  if (provided_callback->is_extern()) {
    return;
  }

  // here we do the following: having PHP code 'array_map(function($v) { ... }, $arr)'
  // with callback_param declared as 'callback ($x ::: ^2[*]) ::: any',
  // create set-edges to infer $v as ^2[*] of $arr on this exact call
  VertexRange callback_args = get_function_params(callback_param);
  for (int i = 0; i < callback_args.size(); ++i) {
    auto callback_ith_arg = callback_args[i].as<op_func_param>(); // $x above
    if (auto type_rule = callback_ith_arg->type_rule) {           // ^2[*] above
      auto call_clone = call.clone();     // in case we have built-ins accepting more than 1 callback
      call_clone->type_rule = type_rule;  // see ExprNodeRecalc::apply_type_rule
      int id_of_callback_argument = provided_callback->is_lambda() ? i + 1 : i;
      create_set(as_lvalue(provided_callback, id_of_callback_argument), call_clone);
    }
  }
}

void CollectMainEdgesPass::on_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->func_id;
  VertexRange function_params = function->get_params();

  if (function->is_extern()) {
    on_func_call_extern_modifying_arg_type(call, function);
  }

  for (int i = 0; i < call->args().size(); ++i) {
    VertexPtr arg = call->args()[i];
    auto param = function_params[i].as<meta_op_func_param>();

    // call an extern function having a callback type description, like 'callback($x ::: ^1[*]) ::: bool'
    if (param->type() == op_func_param_typed_callback) {
      kphp_assert(arg->type() == op_func_ptr);
      on_func_call_param_callback(call, i, arg.as<op_func_ptr>()->func_id);
    }

    // having a f($x) and a call f($arg), it's a bit tricky what's going on here:
    // * if $x declared as '@param ?int', then we don't need even to create an edge —
    //   we create just a postponed type check, as $arg can't influence type of $x, it will remain int|null
    // * but if $x is 'any' or 'array' or some other containing 'any' inside —
    //   we create a set-edge, as we need to infer this 'any' (remember, that $x has a type restriction created in on_function)
    // * the paragraph above about 'any' refers only to PHP functions; built-ins accepting 'any' need only a postponed check
    // * cast params go their own way
    bool should_create_set_to_infer = (!param->type_hint || tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(param->type_hint))->has_tp_any_inside())
                                      && !function->is_extern() && param->type_help == tp_Unknown;

    if (should_create_set_to_infer) {
      create_set(as_lvalue(function, i), arg);
    } else if (param->type_hint) {
      // todo don't create it every time
      create_postponed_type_check(as_rvalue(function, i), as_rvalue(arg), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(param->type_hint)));
    }

    // if a param is passed by reference, function body affects the type of the caller argument:
    // function f(&$x) { $x = 'str'; }     $i = 0; f($i);     - $i will be mixed (string + int)
    // to achieve this, create a reversed edge
    if (param->var()->ref_flag) {
      create_set(as_lvalue(arg), as_rvalue(function, i));
    }
  }
}

void CollectMainEdgesPass::on_return(VertexAdaptor<op_return> v) {
  have_returns = true;

  const FunctionPtr function = current_function;

  // * if it's declared as '@return string', we just need a postponed type check for every return statement
  // * if it's declared as '@return array' of other any-containing — we create a set-edge instead
  // (remember, that a return value has a type restriction if any @return is specified, created in on_function())
  bool should_create_set_to_infer = !function->return_typehint || tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(function->return_typehint))->has_tp_any_inside();

  if (should_create_set_to_infer) {
    create_set(as_lvalue(function, -1), v);
  } else {
    // todo don't create it every time
    create_postponed_type_check(as_rvalue(function, -1), as_rvalue(v), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(function->return_typehint)));
  }
}

void CollectMainEdgesPass::on_foreach(VertexAdaptor<op_foreach> foreach_op) {
  auto params = foreach_op->params();
  auto xs = params->xs();
  auto x = params->x();
  VertexAdaptor<op_var> key;
  if (params->has_key()) {
    key = params->key();
  }
  if (x->ref_flag) {
    create_type_initial_assign(as_lvalue(xs), TypeData::get_type(tp_array));
    create_set(as_lvalue(params), x->var_id);
  } else {
    auto temp_var = params->temp_var().as<op_var>();
    create_set(as_lvalue(temp_var->var_id), xs);
  }
  create_set(as_lvalue(x->var_id), params);
  if (key) {
    create_type_initial_assign(as_lvalue(key->var_id), TypeData::get_type(tp_mixed));
  }
}

void CollectMainEdgesPass::on_list(VertexAdaptor<op_list> list) {
  for (auto cur : list->list()) {
    const auto kv = cur.as<op_list_keyval>();
    // we would like to express array[i] as rvalue multikey int_key, but we lose that
    // key type information during the edges creation (from_node[from_at] = to_node),
    // therefore we have op_index ($kv->var = $list_array[$kv->key]);
    // see the comment above build_real_multikey()
    auto new_v = VertexAdaptor<op_index>::create(list->array(), kv->key());
    new_v.set_location(stage::get_location());
    create_set(as_lvalue(kv->var()), new_v);
  }
}

void CollectMainEdgesPass::on_try(VertexAdaptor<op_try> try_op) {
  for (auto c : try_op->catch_list()) {
    auto catch_op = c.as<op_catch>();
    kphp_assert(catch_op->exception_class);
    create_type_initial_assign(as_lvalue(catch_op->var()), catch_op->exception_class->type_data);
  }
}

// handles expressions that modify v, like v = rhs or ++v (it may be not a variable, but arr[i]*=2 for example)
void CollectMainEdgesPass::on_set_op(VertexPtr v) {
  VertexPtr lval;
  if (auto binary_op = v.try_as<meta_op_binary>()) {
    lval = binary_op->lhs();
  } else if (auto unary_op = v.try_as<meta_op_unary>()) {
    lval = unary_op->expr();
  } else {
    kphp_fail();
  }

  const RValue &rval = as_set_value(v);

  // if it's a variable with type hint (for example, public int|false $prop), we also don't need to create a set-edge, only a postponed check
  // here we try to detect if lval is a variable with type hint or @var declared, looks a bit complicated
  // (it's still an optimization, everything would work without it, always creating set-edges, but a bit slower)
  VertexPtr type_hint;
  VarPtr lhs_var;
  if (auto as_prop = lval.try_as<op_instance_prop>()) { // $a->prop++
    lhs_var = as_prop->var_id;
    type_hint = as_prop->var_id->as_class_instance_field()->type_hint;
  } else if (auto as_var = lval.try_as<op_var>()) {
    if (as_var->var_id->is_class_static_var()) {        // A::$prop++
      lhs_var = as_var->var_id;
      type_hint = as_var->var_id->as_class_static_field()->type_hint;
    }
  }

  bool should_create_set_to_infer = !type_hint || tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(type_hint))->has_tp_any_inside();

  if (should_create_set_to_infer) {
    create_set(as_lvalue(lval), rval);
  } else {
    // todo don't create it every time
    create_postponed_type_check(as_rvalue(lhs_var), as_rvalue(rval), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_lt_type_rule>::create(type_hint)));
  }
}

void CollectMainEdgesPass::ifi_fix(VertexPtr v) {
  is_func_id_t ifi_tp = get_ifi_id(v);
  if (ifi_tp == ifi_error) {
    return;
  }
  for (auto cur : *v) {
    if (auto var = cur.try_as<op_var>()) {
      if (var->var_id->is_constant()) {
        continue;
      }
      if (ifi_tp == ifi_unset) {
        create_type_initial_assign(as_lvalue(var), TypeData::get_type(tp_mixed));
      } else if (ifi_tp == ifi_isset && var->var_id->is_global_var()) {
        create_type_initial_assign(as_lvalue(var), TypeData::get_type(tp_Null));
      }
    }

    if ((cur->type() == op_var && ifi_tp != ifi_unset) || (ifi_tp > ifi_isset && cur->type() == op_index)) {
      tinf::Node *node = tinf::get_tinf_node(cur);
      if (node->isset_flags == 0) {
        create_isset_check(as_rvalue(node));
      }
      node->isset_flags |= ifi_tp;
    }
  }
}

void CollectMainEdgesPass::on_class(ClassPtr klass) {
  klass->members.for_each([&](ClassMemberInstanceField &f) {
    on_var(f.var);
    if (f.type_hint) {
      create_type_assign_with_restriction(as_lvalue(f.var), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(f.type_hint)));
    }
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    on_var(f.var);
    if (f.type_hint) {
      create_type_assign_with_restriction(as_lvalue(f.var), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(f.type_hint)));
    }
  });
}

void CollectMainEdgesPass::on_function(FunctionPtr function) {
  VertexRange params = function->get_params();

  // f(...) ::: string — set the return value to a return tinf node
  // (if a return value is not a type, but ^1 for example, don't do it, it will be calculated for each invocation)
  if (auto rule = function->root->type_rule.try_as<op_common_type_rule>()) {
    if (auto type_of_rule = rule->rule().try_as<op_type_expr_type>()) {
      create_type_initial_assign(as_lvalue(function, -1), TypeData::get_type(type_of_rule->type_help));
    }
  }

  // f($x ::: string) — set the param tinf node for cast params; if array, it means array<any>
  // (for not-cast params — with type hints — it will be done after)
  for (int i = 0; i < params.size(); i++) {
    PrimitiveType ptype = params[i]->type_help;
    if (ptype != tp_Unknown) {
      create_type_initial_assign(as_lvalue(function, i), TypeData::get_type(ptype, tp_Any));
    }
  }

  // all arguments having a type hint or @param — create both set and restriction edges
  for (int i = 0; i < params.size(); i++) {
    if (auto param = params[i].try_as<op_func_param>()) {
      if (param->type_hint) {
        create_type_assign_with_restriction(as_lvalue(function, i), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_lt_type_rule>::create(param->type_hint)));
      }
    }
  }

  // if a function has a @return or type hint — it's also an assign with restriction
  if (function->return_typehint) {
    create_type_assign_with_restriction(as_lvalue(function, -1), tinf::convert_type_rule_to_TypeData(VertexAdaptor<op_set_check_type_rule>::create(function->return_typehint)));
  }
}

void CollectMainEdgesPass::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    on_class(current_function->class_id);
  } else {
    on_function(current_function);
  }
}

VertexPtr CollectMainEdgesPass::on_enter_vertex(VertexPtr v) {
  if (v->type_rule && !current_function->is_extern()) {
    kphp_error(v->type() == op_var, "type_rule can be set only for op_var");
    add_type_rule(v.as<op_var>());
  }
  if (v->type_help != tp_Unknown) {
    kphp_error(v->type() == op_func_param, "type_help can be set only for op_func_param or op_type_expr_type");
  }

  switch (v->type()) {
    case op_func_call:
      on_func_call(v.as<op_func_call>());
      break;
    case op_return:
      on_return(v.as<op_return>());
      break;
    case op_foreach:
      on_foreach(v.as<op_foreach>());
      break;
    case op_list:
      on_list(v.as<op_list>());
      break;
    case op_try:
      on_try(v.as<op_try>());
      break;
    default:
      break;
  }
  if (OpInfo::rl(v->type()) == rl_set || vk::any_of_equal(v->type(), op_prefix_inc, op_postfix_inc, op_prefix_dec, op_postfix_dec)) {
    on_set_op(v);
  }

  ifi_fix(v);

  return v;
}

void CollectMainEdgesPass::on_var(VarPtr var) {
  if (var->tinf_flag) {
    return;
  }
  if (!__sync_bool_compare_and_swap(&var->tinf_flag, false, true)) {
    return;
  }
  tinf::get_inferer()->add_node(tinf::get_tinf_node(var));
  if (var->init_val) {
    create_set(as_lvalue(var), var->init_val);
  }
}

void CollectMainEdgesPass::on_finish() {
  if (current_function->is_extern()) {
    return;
  }
  if (!have_returns) {
    // hack to work well with functions which always throws
    create_type_initial_assign(as_lvalue(current_function, -1), TypeData::get_type(tp_void));
  }
  call_on_var(current_function->local_var_ids);
  call_on_var(current_function->global_var_ids);
  call_on_var(current_function->static_var_ids);
  call_on_var(current_function->implicit_const_var_ids);
  call_on_var(current_function->explicit_const_var_ids);
  call_on_var(current_function->explicit_header_const_var_ids);
  call_on_var(current_function->param_ids);
}

template<class CollectionT>
void CollectMainEdgesPass::call_on_var(const CollectionT &collection) {
  for (const auto &el: collection) {
    on_var(el);
  }
}

// We would like to turn $a[6][$idx] = ... into a multikey (int 6, any) instead of AnyKey(2),
// but the real code contains a lot of numerical indexes which leads to a big memory usage
// increase during the type inference.
//
// This is one of the reasons why tuples are read-only.
// Writing to an index leads to any key type, but reading is OK (see recalc_index in the type inference).
/*
MultiKey *build_real_multikey (VertexPtr v) {
  MultiKey *key = new MultiKey();

  if (v->type() == op_foreach_param) {
    key->push_front(Key::any_key());
    v = v.as <op_foreach_param>()->xs();
  }
  while (v->type() == op_index) {
    if (v.as <op_index>()->has_key() && v.as <op_index>()->key()->type() == op_int_const) {
      key->push_front(Key::int_key(std::atoi(v.as <op_index>()->key()->get_string().c_str())));
    } else {
      key->push_front(Key::any_key());
    }
    v = v.as <op_index>()->array();
  }

  return key;
}
*/
