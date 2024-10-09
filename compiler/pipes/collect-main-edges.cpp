// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-main-edges.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/vertex-util.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/ifi.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-isset.h"
#include "compiler/inferring/restriction-match-phpdoc.h"
#include "compiler/inferring/restriction-non-void.h"
#include "compiler/inferring/type-node.h"
#include "compiler/type-hint.h"
#include "common/php-functions.h"

namespace {

SwitchKind get_switch_kind(VertexAdaptor<op_switch> s) {
  int num_const_int_cases = 0;
  int num_const_string_cases = 0;
  int num_value_cases = 0;

  for (auto one_case : s->cases()) {
    if (one_case->type() == op_default) {
      continue;
    }

    num_value_cases++;
    auto val = VertexUtil::get_actual_value(one_case.as<op_case>()->expr());
    if (VertexUtil::is_const_int(val)) {
      num_const_int_cases++;
    } else if (auto as_string = val.try_as<op_string>()) {
      // PHP would use a numerical comparison for strings that look like a number,
      // we shouldn't rewrite these switches as a string-only switch
      if (!php_is_numeric(as_string->str_val.data())) {
        num_const_string_cases++;
      }
    }
  }

  if (num_value_cases == 0) {
    return SwitchKind::EmptySwitch;
  }

  if (num_const_string_cases == num_value_cases) {
    return SwitchKind::StringSwitch;
  } else if (num_const_int_cases == num_value_cases) {
    return SwitchKind::IntSwitch;
  }
  return SwitchKind::VarSwitch;
}

} // namespace

RValue CollectMainEdgesPass::as_set_value(VertexPtr v) {
  // $lhs = $rhs
  if (v->type() == op_set) {
    return as_rvalue(v.as<op_set>()->rhs());
  }

  // $lhs++ => $lhs = $lhs + 1
  if (vk::any_of_equal(v->type(), op_prefix_inc, op_prefix_dec, op_postfix_dec, op_postfix_inc)) {
    auto unary = v.as<meta_op_unary>();
    auto location = stage::get_location();
    auto one = VertexUtil::create_int_const(1).set_location(location);
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
  return {};
}

// an isset check will analyze expressions that may differ from PHP after getting elements from typed arrays
// for example: if $a is int[], then if($a[100] === 0) may differ from PHP if 100-th element doesn't exist (null vs 0)
void CollectMainEdgesPass::create_isset_check(const RValue &rvalue) {
  tinf::Node *a = rvalue.node;
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionIsset(a));
}

// a 'set edge' from lhs to rhs means, that rhs type is copied to lhs type (see VarNode::do_recalc)
// so, lhs becomes at least the same type as rhs
// lhs is VarNode (for ExprNode this has no effect, as expr nodes are recalculated in another way)
// rhs is an expression (not a type!), so every time rhs is recalculated, lhs is recalculated also
template<class R>
void CollectMainEdgesPass::create_set(const LValue &lhs, const R &rhs) {
  tinf::Edge *edge = new tinf::Edge();
  edge->from = lhs.value;
  edge->from_at = lhs.key;
  edge->to = as_rvalue(rhs).node;
  kphp_assert(edge->to);
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);
}

// an 'initial type assign' means, that lhs type is at least initial_type: it's assigned to it once in a given place
// for example, a key in foreach is mixed
// it is very similar to create_set(), but this function exists in parallel to express another semantics
// btw, this edge also appears in stacktrace on type mismatch, answering "why it was inferred in such a way"
void CollectMainEdgesPass::create_type_assign(const LValue &lhs, const TypeData *initial_type) {
  tinf::Edge *edge = new tinf::Edge();
  edge->from = lhs.value;
  edge->from_at = lhs.key;
  edge->to = new tinf::TypeNode(initial_type, stage::get_location());
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);
}

// when we have array_map(function($v) { ... }, ...), then type of $v depends on this exact call
// it's because array_map's first argument is declared as callable(^2[*] $x) : any
// as an input, we have lhs ($v), func_call (exact array_map invocation), and type_hint for lhs (^2[*])
// it's not a type assign, because tinf::TypeNode holds a constexpr type, whereas here we have a rule with call context
void CollectMainEdgesPass::create_type_assign_with_arg_ref_rule(const LValue &lhs, const TypeHint *type_hint, VertexPtr func_call) {
  auto recalc_trigger = VertexAdaptor<op_trigger_recalc_arg_ref_rule>::create(); // see ExprNodeRecalc
  recalc_trigger->type_hint = type_hint;
  recalc_trigger->func_call = func_call;
  create_set(lhs, recalc_trigger);

  // create dependencies: every time the 2nd argument is recalculated, recalculate this trigger, which will lead to $v recalculation
  create_edges_to_recalc_arg_ref(type_hint, recalc_trigger, func_call);
}

// a 'type assign with restriction' is a phpdoc-controlled type meaning that it is assigned and stays valid during inferring
// for example, function f(?int $x) — then $x is set int|null, and even if passed string to f, $x remains int|null, triggering an error
// if a type contains any, e.g. array $x — then $x is set to any[], this any is inferred via other edges, but passing string triggers an error
// lhs here is VarNode (it means: a variable, or a function param, or a function return)
void CollectMainEdgesPass::create_type_assign_with_restriction(const LValue &var_lhs, const TypeHint *type_hint) {
  auto *restricted_node = dynamic_cast<tinf::VarNode *>(var_lhs.value);
  kphp_assert(restricted_node && var_lhs.key->empty() && type_hint->is_typedata_constexpr());

  // this is just a type assign
  tinf::Edge *edge = new tinf::Edge();
  edge->from = restricted_node;
  edge->from_at = var_lhs.key;
  edge->to = new tinf::TypeNode(type_hint->to_type_data(), stage::get_location());
  tinf::get_inferer()->add_edge(edge);
  tinf::get_inferer()->add_node(edge->from);

  // this is a restriction, consider VarNode for how it works
  restricted_node->set_type_restriction(type_hint->to_type_data());
}

// a 'postponed type check' is a type check without creating a set node
// for example, function f(?int $x) {} f($v) — $x will remain int|null regardless of $v
// therefore, we don't need to create an edge $x->$v, but after tinf has finished, we need to check that type of $v matches int|null
// this is just an optimization to create less set edges: whenever $v is updated, $x recalculation is not triggered
// without this, everything would still work jue to type checks, but a bit slower due to useless recalculations
void CollectMainEdgesPass::create_postponed_type_check(const RValue &restricted_value, const RValue &actual_value, const TypeHint *type_hint) {
  auto *restricted_node = dynamic_cast<tinf::VarNode *>(restricted_value.node);
  auto *actual_node = actual_value.node;
  kphp_assert(restricted_node && actual_node && type_hint->is_typedata_constexpr());
  tinf::get_inferer()->add_node(actual_node);
  tinf::get_inferer()->add_restriction(new RestrictionMatchPhpdoc(restricted_node, actual_node, type_hint->to_type_data()));
}

// a 'non-void check' will test that lhs value is not void
// for example, callback of array_map isn't allowed to return void
void CollectMainEdgesPass::create_non_void(const RValue &lhs) {
  tinf::Node *a = lhs.node;
  tinf::get_inferer()->add_node(a);
  tinf::get_inferer()->add_restriction(new RestrictionNonVoid(a));
}

// create edges for ^1, ^2[*] recalculations in a context of a func call like array_map('cb', $arr);
// every type call_arg ^i is recalculated ($arr for example), dependent_vertex recalculation will be triggered
void CollectMainEdgesPass::create_edges_to_recalc_arg_ref(const TypeHint *type_hint, VertexPtr dependent_vertex, VertexPtr func_call) {
  kphp_assert(type_hint->has_argref_inside());
  type_hint->traverse([func_call, dependent_vertex](const TypeHint *child) {
    VertexPtr call_arg{nullptr};

    if (const auto *as_arg_ref = child->try_as<TypeHintArgRef>()) {
      call_arg = VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, func_call);
    } else if (const auto *as_arg_ref = child->try_as<TypeHintArgRefInstance>()) {
      call_arg = VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, func_call);
    } else if (const auto *as_arg_ref = child->try_as<TypeHintArgRefCallbackCall>()) {
      call_arg = VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, func_call);
    }

    if (call_arg) {
      tinf::Edge *e = new tinf::Edge;
      e->from = tinf::get_tinf_node(dependent_vertex);
      e->to = call_arg->type() == op_callback_of_builtin
              ? tinf::get_tinf_node(call_arg.as<op_callback_of_builtin>()->func_id, -1)
              : tinf::get_tinf_node(call_arg);
      e->from_at = nullptr;
      tinf::get_inferer()->add_edge(e);
      tinf::get_inferer()->add_node(e->to);
    }
  });
}


// handle @var phpdoc for local/static vars inside a function (or global vars, for top-level functions)
void CollectMainEdgesPass::on_var_phpdoc(VertexAdaptor<op_phpdoc_var> var_op) {
  VarPtr var_id = var_op->var()->var_id;
  kphp_assert(var_op->type_hint && var_id);

  // it may appear duplicated @var for the same variable (strange, but such occasions meet in real code)
  if (var_id->tinf_node.type_restriction) {
    kphp_error(are_equal_types(var_id->tinf_node.type_restriction, var_op->type_hint->to_type_data()),
               fmt_format("Different @var for {} exist", var_id->as_human_readable()));
  } else {
    create_type_assign_with_restriction(as_lvalue(var_id), var_op->type_hint);
  }
}

// handles calls to extern functions that modify arg type
// for example, array_push($arr, $v) is like $arr[]=$v
void CollectMainEdgesPass::on_func_call_extern_modifying_arg_type(VertexAdaptor<op_func_call> call, FunctionPtr extern_function) {
  if (extern_function->name == "array_unshift" || extern_function->name == "array_push") {
    VertexRange args = call->args();
    LValue val = as_lvalue(args[0]);

    auto *key = new MultiKey(*val.key);
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

    const TypeHint *type_hint = TypeHintFuture::create(tp_future_queue, TypeHintArgSubkeyGet::create(TypeHintArgRef::create(2)));
    create_type_assign_with_arg_ref_rule(as_lvalue(future_queue_arg), type_hint, call);
  }
}

// handle calls to built-in functions with callbacks:
// array_filter($a, function($v) { ... }), array_filter($a, 'cb') and similar
// (not to php functions with callable arguments! built-in only)
void CollectMainEdgesPass::on_passed_callback_to_builtin(VertexAdaptor<op_func_call> call, int param_i, VertexAdaptor<op_callback_of_builtin> v_callback) {
  FunctionPtr call_function = call->func_id;            // array_filter, etc
  FunctionPtr provided_callback = v_callback->func_id;  // typically, a lambda or __invoke method
  auto callback_param = call_function->get_params()[param_i].as<op_func_param>();
  const auto *type_hint_callable = callback_param->type_hint->try_as<TypeHintCallable>();

  // set restriction of return type of provided callback
  // built-in functions are declared as 'callable(...):any' or 'callable(...):bool'
  // if 'any', it must return anything but void; otherwise, the type rule must be satisfied (it's static, no argument depends)
  const auto *as_primitive = type_hint_callable->return_type->try_as<TypeHintPrimitive>();
  if (as_primitive && as_primitive->ptype == tp_any) {
    create_non_void(as_rvalue(provided_callback, -1));
  } else {
    create_postponed_type_check(as_rvalue(provided_callback, -1), as_rvalue(provided_callback, -1), type_hint_callable->return_type);
  }

  // analyze uncommon pattern 'array_map("array_filter", $arr)': the callback itself has ^1 return typehint
  // so, we need to recalc this call whenever arguments applied for this callback are recalculated
  if (provided_callback->return_typehint && provided_callback->return_typehint->has_argref_inside()) {
    for (const auto *callback_param_type_hint : type_hint_callable->arg_types) {
      if (callback_param_type_hint->has_argref_inside()) {
        create_edges_to_recalc_arg_ref(callback_param_type_hint, call, call);
      }
    }
  }

  // extern functions passed as string-callbacks don't need to be handled further
  if (provided_callback->is_extern()) {
    return;
  }

  // here we do the following: having PHP code 'array_map(function($v) { ... }, $arr)'
  // with callback_param declared as 'callable(^2[*] $x):any',
  // create set-edges to infer $v as ^2[*] of $arr on this exact call
  param_i = 0;
  for (VertexPtr cpp_captured : *v_callback) {
    create_set(as_lvalue(provided_callback, param_i), as_rvalue(cpp_captured));
    param_i++;
  }
  for (int i = 0; i < type_hint_callable->arg_types.size(); ++i, ++param_i) {
    const auto *callback_param_decl = type_hint_callable->arg_types[i]; // ^2[*] above
    if (callback_param_decl->has_argref_inside()) {
      create_type_assign_with_arg_ref_rule(as_lvalue(provided_callback, param_i), callback_param_decl, call);
    } else {
      create_type_assign(as_lvalue(provided_callback, param_i), callback_param_decl->to_type_data());
    }
  }
}

void CollectMainEdgesPass::on_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->func_id;
  VertexRange function_params = function->get_params();

  if (function->is_extern()) {
    on_func_call_extern_modifying_arg_type(call, function);
  }

  // if this function returns ^1, we need to recalc this call vertex every time the 1st argument is recalculated
  if (function->return_typehint && function->return_typehint->has_argref_inside()) {
    create_edges_to_recalc_arg_ref(function->return_typehint, call, call);
  }

  for (int i = 0; i < call->args().size(); ++i) {
    VertexPtr arg = call->args()[i];
    auto param = function_params[i].as<op_func_param>();
    bool is_cast_param = !stage::get_function()->file_id->is_strict_types && param->is_cast_param;
    // call an extern function having a callback type description, like 'callable(^1[*]) : bool'
    if (function->is_extern() && param->type_hint && param->type_hint->try_as<TypeHintCallable>()) {
      // for FFI calls, php2c(null) is used to express a null function pointer
      if (arg->type() != op_ffi_php2c_conv) {
        kphp_assert(arg->type() == op_callback_of_builtin);
        on_passed_callback_to_builtin(call, i, arg.as<op_callback_of_builtin>());
      }
    }

    // having a f($x) and a call f($arg), it's a bit tricky what's going on here:
    // * if $x declared as '@param ?int', then we don't need even to create an edge —
    //   we create just a postponed type check, as $arg can't influence type of $x, it will remain int|null
    // * but if $x is 'any' or 'array' or some other containing 'any' inside —
    //   we create a set-edge, as we need to infer this 'any' (remember, that $x has a type restriction created in on_function)
    // * the paragraph above about 'any' refers only to PHP functions; built-ins accepting 'any' need only a postponed check
    // * cast params go their own way
    bool should_create_set_to_infer = (!param->type_hint || param->type_hint->has_tp_any_inside())
                                      && !function->is_extern() && !is_cast_param;

    if (should_create_set_to_infer) {
      create_set(as_lvalue(function, i), arg);
    } else if (param->type_hint && param->type_hint->is_typedata_constexpr() && !is_cast_param) {
      create_postponed_type_check(as_rvalue(function, i), as_rvalue(arg), param->type_hint);
    }

    // if a param is passed by reference, function body affects the type of the caller argument:
    // function f(&$x) { $x = 'str'; }     $i = 0; f($i);     - $i will be mixed (string + int)
    // to achieve this, create a reversed edge
    if (param->var()->ref_flag) {
      create_set(as_lvalue(arg), as_rvalue(function, i));
    }
  }
}

void CollectMainEdgesPass::on_return(VertexAdaptor<op_return> v, FunctionPtr function) {
  // * if it's declared as '@return string', we just need a postponed type check for every return statement
  // * if it's declared as '@return array' of other any-containing — we create a set-edge instead
  // (remember, that a return value has a type restriction if any @return is specified, created in on_function())
  bool should_create_set_to_infer = !function->return_typehint || function->return_typehint->has_tp_any_inside();

  if (should_create_set_to_infer) {
    create_set(as_lvalue(function, -1), v);
  } else {
    create_postponed_type_check(as_rvalue(function, -1), as_rvalue(v), function->return_typehint);
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
    create_type_assign(as_lvalue(xs), TypeData::get_type(tp_array));
    create_set(as_lvalue(params), x->var_id);
  } else {
    auto temp_var = params->temp_var().as<op_var>();
    create_set(as_lvalue(temp_var->var_id), xs);
  }
  create_set(as_lvalue(x->var_id), params);
  if (key) {
    create_type_assign(as_lvalue(key->var_id), TypeData::get_foreach_key_type());
  }
}

void CollectMainEdgesPass::on_switch(VertexAdaptor<op_switch> switch_op) {
  // if there is a switch that will compile as var-switch,
  // try to deduce the type of switch cond variable;
  // to avoid the increased compile-time without any benefits, handle
  // int-only and string-only switches separately (these simple switch statements
  // form a majority of all switch statements)
  switch_op->kind = get_switch_kind(switch_op);
  if (switch_op->kind == SwitchKind::IntSwitch) {
    // in case of int-only switch, condition var is discarded, so there is
    // no real need in trying to insert an assignment node here
    create_type_assign(as_lvalue(switch_op->condition_on_switch()), TypeData::get_type(tp_int));
  } else if (switch_op->kind == SwitchKind::StringSwitch) {
    // when switch cond is assigned, it's done via strval(expr()), so it will be string-typed;
    // therefore, we don't need to insert an assignment node here
    create_type_assign(as_lvalue(switch_op->condition_on_switch()), TypeData::get_type(tp_string));
  } else if (switch_op->kind != SwitchKind::EmptySwitch) {
    // the condition variable should be lca of all case expression types and the condition expr result type itself
    create_set(as_lvalue(switch_op->condition_on_switch()->var_id), switch_op->condition());
    for (const auto &c : switch_op->cases()) {
      if (auto as_case = c.try_as<op_case>()) {
        create_set(as_lvalue(switch_op->condition_on_switch()->var_id), as_case->expr());
      }
    }
  } else {
    create_type_assign(as_lvalue(switch_op->condition_on_switch()), TypeData::get_type(tp_mixed));
  }
  create_type_assign(as_lvalue(switch_op->matched_with_one_case()), TypeData::get_type(tp_bool));
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
    create_type_assign(as_lvalue(catch_op->var()), catch_op->exception_class->type_data);
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
  const TypeHint *type_hint{nullptr};
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

  bool should_create_set_to_infer = !type_hint || type_hint->has_tp_any_inside();

  if (should_create_set_to_infer) {
    create_set(as_lvalue(lval), rval);
  } else {
    create_postponed_type_check(as_rvalue(lhs_var), as_rvalue(rval), type_hint);
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
        create_type_assign(as_lvalue(var), TypeData::get_type(tp_mixed));
      } else if (ifi_tp == ifi_isset && var->var_id->is_global_var()) {
        create_type_assign(as_lvalue(var), TypeData::get_type(tp_Null));
      }
    }

    const auto is_indexing_func_call = [](VertexPtr v) {
      return v->type() == op_func_call && v.as<op_func_call>()->func_id->is_result_indexing;
    };
    if ((cur->type() == op_var && ifi_tp != ifi_unset) || (ifi_tp > ifi_isset && (cur->type() == op_index || is_indexing_func_call(cur)))) {
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
      create_type_assign_with_restriction(as_lvalue(f.var), f.type_hint);
    }
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    on_var(f.var);
    if (f.type_hint) {
      create_type_assign_with_restriction(as_lvalue(f.var), f.type_hint);
    }
  });
}

void CollectMainEdgesPass::on_function(FunctionPtr function) {
  VertexRange params = function->get_params();

  // all arguments having a type hint or @param — create both set and restriction edges
  for (int i = 0; i < params.size(); i++) {
    if (auto param = params[i].try_as<op_func_param>()) {
      if (param->type_hint && param->type_hint->is_typedata_constexpr()) {
        create_type_assign_with_restriction(as_lvalue(function, i), param->type_hint);
      }
    }
  }

  // if a function has a @return or type hint — it's also an assign with restriction
  if (function->return_typehint && function->return_typehint->is_typedata_constexpr()) {
    create_type_assign_with_restriction(as_lvalue(function, -1), function->return_typehint);
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
  switch (v->type()) {
    case op_func_call:
      on_func_call(v.as<op_func_call>());
      break;
    case op_return:
      have_returns = true;
      on_return(v.as<op_return>(), current_function);
      break;
    case op_foreach:
      on_foreach(v.as<op_foreach>());
      break;
    case op_switch:
      on_switch(v.as<op_switch>());
      break;
    case op_list:
      on_list(v.as<op_list>());
      break;
    case op_try:
      on_try(v.as<op_try>());
      break;
    case op_phpdoc_var:
      on_var_phpdoc(v.as<op_phpdoc_var>());
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
    // init_val can contain calls to pure functions, which can have ^1 returns, so edges need to be created
    search_for_nested_calls(var->init_val);
  }
}

void CollectMainEdgesPass::search_for_nested_calls(VertexPtr v) {
  if (auto call = v.try_as<op_func_call>()) {
    on_func_call(call);
  }

  for (auto child : *v) {
    search_for_nested_calls(child);
  }
}

void CollectMainEdgesPass::on_finish() {
  if (current_function->is_extern()) {
    return;
  }
  if (!have_returns && !current_function->return_typehint) {
    // work well with functions which always throws
    create_type_assign(as_lvalue(current_function, -1), TypeData::get_type(tp_void));
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
