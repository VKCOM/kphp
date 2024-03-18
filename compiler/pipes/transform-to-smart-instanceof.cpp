// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/transform-to-smart-instanceof.h"

#include "common/algorithms/compare.h"

#include "compiler/name-gen.h"
#include "compiler/vertex-util.h"

bool TransformToSmartInstanceofPass::user_recursion(VertexPtr v) {
  if (v->type() == op_if) {
    return on_if_user_recursion(v.as<op_if>());
  } else if (v->type() == op_catch) {
    return on_catch_user_recursion(v.as<op_catch>());
  } else if (v->type() == op_lambda) {
    return on_lambda_user_recursion(v.as<op_lambda>());
  }

  return false;
}

// 1) handle `if ($x instanceof A)`, replace $x with a temp var inside its body
// 2) handle `if (!($x instanceof A)) return`, replace $x with a temp var till the end of function
bool TransformToSmartInstanceofPass::on_if_user_recursion(VertexAdaptor<op_if> v_if) {
  auto v_instanceof = get_instanceof_from_if_condition(v_if->cond());
  if (!v_instanceof) {
    if (auto v_not_instanceof = get_instanceof_from_if_not_condition(v_if->cond())) {
      auto instance_var = v_not_instanceof->lhs().as<op_var>().clone();
      return try_replace_if_not_instanceof_return(v_if, instance_var, v_not_instanceof->rhs());
    }
    return false;
  }
  auto instance_var = v_instanceof->lhs().as<op_var>().clone();

  run_function_pass(v_if->cond(), this);

  add_tmp_var_with_instance_cast(instance_var, v_instanceof->rhs(), v_if->true_cmd_ref());
  new_names_of_var[instance_var->get_string()].pop();
  on_enter_vertex(instance_var);

  if (v_if->has_false_cmd()) {
    run_function_pass(v_if->false_cmd_ref(), this);
  }

  return true;
}

// handle `catch (SomeExceptionClass $e)`, replace $e with a different name
// this allows using multiple catch cases in a single try with the same variable name $e
// (without this, assumptions will be mixed)
bool TransformToSmartInstanceofPass::on_catch_user_recursion(VertexAdaptor<op_catch> v_catch) {
  auto instance_var = v_catch->var();
  auto v_exception = VertexAdaptor<op_var>::create().set_location(v_catch);
  v_exception->str_val = gen_unique_name("exception");
  new_names_of_var[instance_var->str_val].push(v_exception->str_val);
  v_catch->var_ref() = v_exception;
  run_function_pass(v_catch->cmd_ref(), this);
  new_names_of_var[instance_var->str_val].pop();

  return true;
}

// handle `fn() => $captured_var` — maybe, $captured_var from uses_list is modified (i.e. from smart instanceof or catch)
bool TransformToSmartInstanceofPass::on_lambda_user_recursion(VertexAdaptor<op_lambda> v_lambda) {
  FunctionPtr f_lambda = v_lambda->func_id;
  for (auto &var_as_use : f_lambda->uses_list) {
    var_as_use = on_enter_vertex(var_as_use).as<op_var>();
  }
  run_function_pass(f_lambda->root, this);

  return true;
}

// having `if (!($x instanceof A)) return;`, replace $x with $tmp_A till the end of function
// we support this only if this `if` is the top-level statement of function, as we don't have cfg at this point
bool TransformToSmartInstanceofPass::try_replace_if_not_instanceof_return(VertexAdaptor<op_if> v_if_not, VertexAdaptor<op_var> instance_var,
                                                                          VertexPtr name_of_derived) {
  bool is_if_not_instanceof_return = !v_if_not->has_false_cmd() && v_if_not->true_cmd()->size() == 1 && v_if_not->true_cmd()->front()->type() == op_return;
  if (!is_if_not_instanceof_return) {
    return false;
  }

  for (auto &child : *current_function->root->cmd()) {
    if (child == v_if_not) {
      run_function_pass(v_if_not->cond(), this);
      run_function_pass(v_if_not->true_cmd_ref(), this);
      // add `else` statement with $tmp_A = instance_cast($x, A::class)
      VertexPtr v_else_assign = VertexAdaptor<op_seq>::create();
      add_tmp_var_with_instance_cast(instance_var, name_of_derived, v_else_assign);
      auto v_if_else = VertexAdaptor<op_if>::create(v_if_not->cond(), v_if_not->true_cmd(), v_else_assign.as<op_seq>());
      child = v_if_else;
      return true;
      // $x will be replaced till the end of function body automatically, as we don't clear new_names_of_var
    }
  }
  return false;
}

void TransformToSmartInstanceofPass::add_tmp_var_with_instance_cast(VertexAdaptor<op_var> instance_var, VertexPtr name_of_derived, VertexPtr &cmd) {
  auto set_instance_cast_to_tmp = generate_tmp_var_with_instance_cast(instance_var, name_of_derived);
  auto &name_of_tmp_var = set_instance_cast_to_tmp->lhs().as<op_var>()->str_val;
  new_names_of_var[instance_var->str_val].push(name_of_tmp_var);

  cmd = VertexAdaptor<op_seq>::create(set_instance_cast_to_tmp, cmd.as<op_seq>()->args()).set_location(cmd);
  auto commands = cmd.as<op_seq>()->args();

  std::for_each(std::next(commands.begin()), commands.end(), [&](VertexPtr &v) { return run_function_pass(v, this); });
}

VertexPtr TransformToSmartInstanceofPass::on_enter_vertex(VertexPtr v) {
  if (auto v_switch = v.try_as<op_switch>()) {
    return try_replace_switch_when_constexpr(v_switch);
  }

  if (v->type() != op_var || new_names_of_var.empty()) {
    return v;
  }

  auto replacement_it = new_names_of_var.find(v->get_string());
  if (replacement_it != new_names_of_var.end() && !replacement_it->second.empty()) {
    v->set_string(replacement_it->second.top());
  }

  return v;
}

VertexAdaptor<op_set> TransformToSmartInstanceofPass::generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex) {
  auto cast_to_derived = VertexAdaptor<op_func_call>::create(instance_var, derived_name_vertex).set_location(instance_var);
  cast_to_derived->str_val = "instance_cast";

  auto tmp_var = VertexAdaptor<op_var>::create().set_location(instance_var);
  tmp_var->set_string(gen_unique_name(instance_var->get_string()));
  tmp_var->extra_type = op_ex_var_superlocal;
  tmp_var->is_const = true;

  auto set_instance_cast_to_tmp = VertexAdaptor<op_set>::create(tmp_var, cast_to_derived).set_location(instance_var);
  derived_name_vertex.set_location(instance_var);

  return set_instance_cast_to_tmp;
}

// try to find `if ($x instanceof A)`, which ast is op_conv_bool > op_instanceof
VertexAdaptor<op_instanceof> TransformToSmartInstanceofPass::get_instanceof_from_if_condition(VertexPtr if_cond) {
  if (auto condition = if_cond.try_as<op_conv_bool>()) {
    if (auto as_instanceof = condition->expr().try_as<op_instanceof>()) {
      if (as_instanceof->lhs()->type() == op_var) {
        return as_instanceof;
      }
    }
  }

  return {};
}

// try to find `if (!($x instanceof A))`, which ast is op_conv_bool > op_log_not > op_conv_bool > op_instanceof
VertexAdaptor<op_instanceof> TransformToSmartInstanceofPass::get_instanceof_from_if_not_condition(VertexPtr if_cond) {
  if (auto cond_wrap = if_cond.try_as<op_conv_bool>()) {
    if (auto as_not = cond_wrap->expr().try_as<op_log_not>()) {
      return get_instanceof_from_if_condition(as_not->expr());
    }
  }

  return {};
}

// switch() for constexpr condition with constexpr cases can be replaced: all false cases dropped off
// it's done early in pipeline (before method binding), so false cases can contain invalid code, it's intentional
// the main purpose is for generic functions with class-string<T>, then $class_name is compile-time known:
// `switch($class_name) case *::class`, different cases can be incompatible as a whole, but okay in every specialization
VertexPtr TransformToSmartInstanceofPass::try_replace_switch_when_constexpr(VertexAdaptor<op_switch> v_switch) {
  VertexPtr v_cond = VertexUtil::get_actual_value(v_switch->condition());
  if (v_cond->type() != op_string) {
    return v_switch;
  }

  // check that
  // 1) all cases are const strings
  // 2) every case ends with `break`/`return`/`throw` or is empty (just fallthrough)
  VertexRange cases = v_switch->cases();
  bool is_valid_const_string_switch =
    cases.size() > 1 && vk::all_of(cases, [](VertexPtr v) {
      auto as_case = v.try_as<op_case>();
      if (!as_case) { // default
        return true;
      }
      VertexPtr last_stmt = as_case->cmd()->empty() ? (VertexPtr)VertexAdaptor<op_empty>::create() : as_case->cmd()->args()[as_case->cmd()->size() - 1];
      return VertexUtil::get_actual_value(as_case->expr())->type() == op_string && vk::any_of_equal(last_stmt->type(), op_break, op_return, op_throw);
    });
  if (!is_valid_const_string_switch) {
    return v_switch;
  }

  // find case x where x == switch condition (true case)
  auto it_true_case = std::find_if(cases.begin(), cases.end(), [v_cond](VertexPtr v_case) {
    return v_case->type() == op_case && VertexUtil::get_actual_value(v_case.as<op_case>()->expr())->get_string() == v_cond->get_string();
  });
  // for 'case x: case y: ...', fall through empty cases
  while (it_true_case != cases.end() && (*it_true_case)->type() == op_case && it_true_case->as<op_case>()->cmd()->empty()) {
    ++it_true_case;
  }

  // get body of true case
  VertexAdaptor<op_seq> v_true_cmd;
  if (it_true_case != cases.end() && (*it_true_case)->type() == op_case) {
    v_true_cmd = it_true_case->as<op_case>()->cmd();
  } else {
    auto it_default = std::find_if(cases.begin(), cases.end(), [](VertexPtr v_case) { return v_case->type() == op_default; });
    if (it_default != cases.end()) {
      v_true_cmd = it_default->as<op_default>()->cmd();
    }
  }
  if (!v_true_cmd) {
    std::vector<VertexPtr> v_empty = {VertexAdaptor<op_empty>::create().set_location(v_switch)};
    v_true_cmd = VertexAdaptor<op_seq>::create(v_empty).set_location(v_switch);
  }

  // return `switch(condition) default: {body of true 'case'}`
  // (it's easier to reconstruct another switch with only-default branch then to eliminate 'break;' inside)
  auto ins_default = VertexAdaptor<op_default>::create(v_true_cmd).set_location(v_true_cmd);
  return VertexUtil::create_switch_vertex(current_function, v_switch->condition(), {ins_default});
}
