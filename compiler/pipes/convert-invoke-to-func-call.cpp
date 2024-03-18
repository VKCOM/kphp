// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-invoke-to-func-call.h"

#include "compiler/name-gen.h"
#include "compiler/type-hint.h"

/*
 * This pass does creates op_func_call that can't be created earlier: at this point, we are after a sync point.
 * - replaces op_invoke_call `$f(...)` with op_func_call (typically, resulting in `$f->__invoke(...)`)
 *   (can't be done earlier, until _all_ op_lambda are wrapped to lambda classes)
 * - deals with `__virt_clone` and similar
 *   (can't be done earlier, until all virtual functions' bodies are represented with switch-case, typed callables also)
 * - binds func_id to op_callback_of_builtin
 *   (can't be done earlier, as it can point to __invoke and therefore needs a lambda to be wrapped with a class)
 */

// on_exit_vertex (not on enter) is important, see comments below
VertexPtr ConvertInvokeToFuncCallPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_clone) {
    stage::set_location(root->location);
    return on_clone(root.as<op_clone>());
  } else if (root->type() == op_callback_of_builtin) {
    stage::set_location(root->location);
    return on_callback_of_builtin(root.as<op_callback_of_builtin>());
  } else if (root->type() == op_func_call) {
    stage::set_location(root->location);
    return on_func_call(root.as<op_func_call>());
  } else if (root->type() == op_invoke_call) {
    stage::set_location(root->location);
    return on_invoke_call(root.as<op_invoke_call>());
  }

  return root;
}

// op_callback_of_builtin is an argument passed as a callback to an extern function
// * `array_map('strlen')` where 'strlen' was wrapped into op_callback_of_builtin with str_val=strlen
// * `array_map(fn(...)=>...)` where a lambda was wrapped into op_callback_of_builtin with bound_instance=lambda_class and str_val=__invoke
// * `array_map($callback)` where a variable is wrapper into op_callback_of_builtin with bound_instance=$callback and str_val=__invoke
// here we set func_id of this op_callback_of_builtin
// this can be done only here, after all op_lambda have been instantiated to lambda classes, and assumptions now work for them
VertexPtr ConvertInvokeToFuncCallPass::on_callback_of_builtin(VertexAdaptor<op_callback_of_builtin> v_callback) {
  // func_id is already set only in one case: when a lambda was passed to a built-in function
  // then a lambda's uses_list is already captured by v_callback — so, all work is already done, just exit
  if (v_callback->func_id) {
    kphp_assert(v_callback->func_id->is_lambda());
    nested_lambdas.emplace_front(v_callback->func_id);
    return v_callback;
  }
  // otherwise, func_id is not set, and we need to set it here

  if (v_callback->args().empty()) {
    FunctionPtr f_points_to = G->get_function(v_callback->str_val);
    kphp_error_act(f_points_to, fmt_format("Can't find function {}", v_callback->str_val), return v_callback);
    kphp_error(f_points_to->is_required,
               fmt_format("{} needs the @kphp-required tag, because it's used only as a string callback", f_points_to->as_human_readable()));

    v_callback->func_id = f_points_to;
    return v_callback;
  }

  auto bound_this = v_callback->args()[0];
  ClassPtr c_assumed = assume_class_of_expr(current_function, bound_this, v_callback).try_as_class();
  const auto *class_method = c_assumed ? c_assumed->get_instance_method(v_callback->str_val) : nullptr;
  kphp_error_act(class_method, "Invalid callback passed, could not detect a class or its method", return v_callback);

  v_callback->func_id = class_method->function;
  return v_callback;
}

// up to this point, all `$f = function() { ... }`
// have already been replaced with `$f = Lambda$xxx$$__construct(op_alloc)`
// such lambda classes have __invoke() method that actually calls the original lambda (see lambda-utils.cpp)
// detect such constructions to fill `this->nested_lambdas`
VertexPtr ConvertInvokeToFuncCallPass::on_func_call(VertexAdaptor<op_func_call> v_call) {
  FunctionPtr called_f = v_call->func_id;

  if (called_f->class_id && called_f->class_id->is_lambda_class() && called_f->is_constructor()) {
    FunctionPtr lambda_f = called_f->class_id->get_instance_method("__invoke")->function->outer_function;
    nested_lambdas.emplace_front(lambda_f);
  }

  return v_call;
}

// here we replace op_invoke_call `$f(...)` with an op_func_call `$f->__invoke(...)`
// this can be done only here (not in deducing types), after all lambdas have been instantiated
VertexPtr ConvertInvokeToFuncCallPass::on_invoke_call(VertexAdaptor<op_invoke_call> v_invoke_call) {
  ClassPtr klass = assume_class_of_expr(current_function, v_invoke_call->args()[0], v_invoke_call).try_as_class();
  kphp_error_act(klass && (klass->is_lambda_class() || klass->is_typed_callable_interface()), "Invalid ()-invocation of a non-lambda", return v_invoke_call);

  auto v_func_call = VertexAdaptor<op_func_call>::create(v_invoke_call->args()).set_location(v_invoke_call);
  v_func_call->extra_type = op_ex_func_call_arrow;
  v_func_call->func_id = klass->get_instance_method("__invoke")->function;
  v_func_call->str_val = "__invoke";

  return v_func_call;
}

// here we replace `clone $obj` with __virt_clone or __clone magic method on demand
// we do it here, after all virtual methods have been generated
// note, that on_exit_vertex (not on_enter_vertex) is significant to bypass recursion
VertexPtr ConvertInvokeToFuncCallPass::on_clone(VertexAdaptor<op_clone> v_clone) {
  ClassPtr klass = v_clone->class_id;
  bool inside_virt_clone =
    vk::any_of_equal(current_function->local_name(), ClassData::NAME_OF_VIRT_CLONE, FunctionData::get_name_of_self_method(ClassData::NAME_OF_VIRT_CLONE));

  // clone $obj, when $obj is a parent class/interface, is replaced with $obj->__virt_clone()
  if (!klass->derived_classes.empty() && !inside_virt_clone) {
    auto call_virt_clone = VertexAdaptor<op_func_call>::create(v_clone->expr()).set_location(v_clone);
    call_virt_clone->extra_type = op_ex_func_call_arrow;
    call_virt_clone->str_val = ClassData::NAME_OF_VIRT_CLONE;
    call_virt_clone->func_id = klass->members.get_instance_method(ClassData::NAME_OF_VIRT_CLONE)->function;

    return call_virt_clone;
  }

  // clone $obj, when a class has the __clone() magic method, is replaced with { tmp = clone $obj; tmp->__clone(); $tmp }
  if (klass->members.has_instance_method(ClassData::NAME_OF_CLONE)) {
    auto tmp_var = VertexAdaptor<op_var>::create().set_location(v_clone);
    tmp_var->str_val = gen_unique_name("tmp_for_clone");
    tmp_var->extra_type = op_ex_var_superlocal;

    auto set_clone_to_tmp = VertexAdaptor<op_set>::create(tmp_var, v_clone).set_location(v_clone);

    auto call_magic_clone = VertexAdaptor<op_func_call>::create(tmp_var).set_location(v_clone);
    call_magic_clone->str_val = ClassData::NAME_OF_CLONE;
    call_magic_clone->extra_type = op_ex_func_call_arrow;
    call_magic_clone->func_id = klass->members.get_instance_method(ClassData::NAME_OF_CLONE)->function;

    return VertexAdaptor<op_seq_rval>::create(set_clone_to_tmp, call_magic_clone, tmp_var).set_location(v_clone);
  }

  // otherwise, it's left as is: clone $obj
  return v_clone;
}

// ConvertInvokeToFuncCallF is needed to hold lambdas so they don't pass the pipeline before a containing function
void ConvertInvokeToFuncCallF::execute(FunctionPtr f, DataStream<FunctionPtr> &os) {
  if (f->is_lambda()) {
    // don't push it into pipeline
    // it will be pushed (or already was pushed) by a function containing this lambda
    return;
  }

  execute_with_nested_lambdas(f, os);
}

// whenever f is pushed to the pipeline, all lambdas within it have already been processed
void ConvertInvokeToFuncCallF::execute_with_nested_lambdas(FunctionPtr f, DataStream<FunctionPtr> &os) {
  ConvertInvokeToFuncCallPass pass;
  run_function_pass(f, &pass);
  for (FunctionPtr f_lambda : pass.flush_nested_lambdas()) {
    execute_with_nested_lambdas(f_lambda, os);
  }

  os << f;
}
