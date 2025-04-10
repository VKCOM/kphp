// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/instantiate-generics-and-lambdas.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/lambda-utils.h"
#include "compiler/phpdoc.h"
#include "compiler/pipes/clone-nested-lambdas.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"

/*
 * This pass creates new functions and passes them backwards to be handled again:
 * 1) it generates lambda classes for every op_lambda,
 *    and therefore pipes __construct and __invoke of that class
 * 2) it instantiates generic functions knowing T1, etc
 *    and therefore pipes a newly-created function (an instantiated clone of generic function)
 *    (note, that in InstantiateGenericFunctionPass, func_id and similar are not set: f is yet to pass the pipeline)
 */

class InstantiateGenericFunctionPass final : public FunctionPassBase {
  FunctionPtr generic_function;
  const GenericsInstantiationMixin* instantiationTs;

public:
  InstantiateGenericFunctionPass(FunctionPtr generic_function, const GenericsInstantiationMixin* instantiationTs)
      : generic_function(generic_function),
        instantiationTs(instantiationTs) {}

  std::string get_description() override {
    return "Instantiate generic function";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (auto as_phpdoc_var = root.try_as<op_phpdoc_var>()) {
      // inside f<A>, replace `@var T[] $v` with `@var A[] $v`
      as_phpdoc_var->type_hint = phpdoc_replace_genericTs_with_reified(as_phpdoc_var->type_hint, instantiationTs);

    } else if (auto as_call = root.try_as<op_func_call>()) {
      if (as_call->reifiedTs) {
        kphp_assert(as_call->reifiedTs->empty() && as_call->reifiedTs->commentTs);
        as_call->reifiedTs = new GenericsInstantiationMixin(*as_call->reifiedTs);
        for (auto& type_hint : as_call->reifiedTs->commentTs->vectorTs) {
          type_hint = phpdoc_replace_genericTs_with_reified(type_hint, instantiationTs);
        }
      }
      // inside f<A>, replace `classof($o)` with "A" if `f<T>(T $o)` called as `f(expr_assumed_A)`
      if (as_call->str_val == "classof" && as_call->size() == 1 && as_call->extra_type != op_ex_func_call_arrow) {
        if (auto as_var = as_call->args().begin()->try_as<op_var>()) {
          if (const TypeHint* param_type_hint = get_param_type_hint_if_generic(as_var->str_val)) {
            if (const auto* as_genericT = param_type_hint->try_as<TypeHintGenericT>()) {
              return replace_with_string_const_if_T_is_class(as_genericT->nameT, root);
            }
          }
        }
      }

    } else if (auto as_var = root.try_as<op_var>()) {
      // inside f<A>, replace `$cn` with "A" if `f<T>(class-string<T> $cn)` called as `f(A::class)`
      if (const TypeHint* param_type_hint = get_param_type_hint_if_generic(as_var->str_val)) {
        if (const auto* as_class_string = param_type_hint->try_as<TypeHintClassString>()) {
          return replace_with_string_const_if_T_is_class(as_class_string->inner->try_as<TypeHintGenericT>()->nameT, root);
        }
      }

    } else if (auto as_op_lambda = root.try_as<op_lambda>()) {
      InstantiateGenericFunctionPass pass(generic_function, instantiationTs);
      run_function_pass(as_op_lambda->func_id, &pass);
    }

    return root;
  }

  bool user_recursion(VertexPtr root) override {
    if (root->type() == op_func_param_list) {
      return true;
    }

    return false;
  }

  VertexPtr replace_with_string_const_if_T_is_class(const std::string& nameT, VertexPtr v_to_replace) {
    if (const TypeHint* instT = instantiationTs->find(nameT)) {
      if (const auto* as_instance = instT->try_as<TypeHintInstance>()) {
        return VertexUtil::create_string_const(as_instance->full_class_name).set_location(v_to_replace);
      }
    }
    return v_to_replace;
  }

  const TypeHint* get_param_type_hint_if_generic(const std::string& var_name) {
    auto param = generic_function->find_param_by_name(var_name);
    if (param && param->type_hint && param->type_hint->has_genericT_inside() && find_param_by_name(var_name)) {
      return param->type_hint;
    }
    return nullptr;
  }

  VertexAdaptor<op_func_param> find_param_by_name(const std::string& var_name) {
    FunctionPtr outer_function = current_function;
    while (outer_function->is_lambda()) {
      bool is_used = std::find_if(outer_function->uses_list.begin(), outer_function->uses_list.end(),
                                  [var_name](auto v_use) { return v_use->str_val == var_name; }) != outer_function->uses_list.end();
      if (!is_used) {
        return {};
      }
      outer_function = outer_function->outer_function;
    }
    return outer_function->find_param_by_name(var_name);
  }
};

// when we have a call `f<T1, T2>(...)`, we generate a new function `f$_$T1$_$T2`,
// which is not a generic one, but an instantiated one
static FunctionPtr instantiate_generic_function(FunctionPtr generic_function, GenericsInstantiationMixin* instantiationTs,
                                                const std::string& name_of_instantiated_function) {

  auto new_function = FunctionData::clone_from(generic_function, name_of_instantiated_function);
  CloneNestedLambdasPass::run_if_lambdas_inside(new_function, nullptr);
  new_function->genericTs = nullptr;
  new_function->instantiationTs = instantiationTs;
  new_function->outer_function = generic_function;

  // replace all T inside a cloned function where it could potentially be:
  // 1) in type_hint of params/return
  // 2) in nested @var phpdocs and nested /*<T>*/ calls

  for (auto p : new_function->get_params()) {
    auto param = p.as<op_func_param>();
    if (param->type_hint) {
      param->type_hint = phpdoc_replace_genericTs_with_reified(param->type_hint, instantiationTs);
    }
  }

  if (new_function->return_typehint && new_function->return_typehint->has_genericT_inside()) {
    new_function->return_typehint = phpdoc_replace_genericTs_with_reified(new_function->return_typehint, instantiationTs);
  }

  InstantiateGenericFunctionPass pass(generic_function, instantiationTs);
  run_function_pass(new_function, &pass);

  kphp_assert(!new_function->is_required && !new_function->is_generic());
  return new_function;
}

static FunctionPtr instantiate_generic_function_concurrent(FunctionPtr generic_function, GenericsInstantiationMixin* instantiationTs,
                                                           const std::string& name_of_instantiated_function, DataStream<FunctionPtr>& function_stream) {
  FunctionPtr generated_function;

  G->operate_on_function_locking(name_of_instantiated_function, [&](FunctionPtr& f) {
    if (!f) {
      f = instantiate_generic_function(generic_function, instantiationTs, name_of_instantiated_function);

      if (f->modifiers.is_instance()) {
        AutoLocker locker(&(*f->class_id));
        f->class_id->members.add_instance_method(f);
      }

      G->require_function(f, function_stream);
    }

    generated_function = f;
  });

  return generated_function;
}

// the pass which is called from the F pipe
// (the F pipe is needed to have several outputs, which is impossible with Pass structure)
class InstantiateGenericsAndLambdasPass final : public FunctionPassBase {
  DataStream<FunctionPtr>& function_stream;
  DataStream<FunctionPtr>& forward_next_stream;

public:
  explicit InstantiateGenericsAndLambdasPass(DataStream<FunctionPtr>& function_stream, DataStream<FunctionPtr>& forward_next_stream)
      : function_stream(function_stream),
        forward_next_stream(forward_next_stream) {}

  std::string get_description() override {
    return "Instantiate generics and lambdas";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (root->type() == op_lambda && !stage::has_error()) {
      return on_op_lambda_convert_to_construct_lambda_class(root.as<op_lambda>());
    }
    if (root->type() == op_callback_of_builtin) {
      return on_op_callback_of_builtin_patch_if_lambda(root.as<op_callback_of_builtin>());
    }

    if (auto call = root.try_as<op_func_call>()) {
      if (call->func_id && call->func_id->is_generic()) {
        call->func_id = on_generic_func_call_instantiate(call->func_id, call);
      }
    }

    return root;
  }

  // have `f<T>(...)`, create f$_$T and return it
  // note, that we always know T due to the previous pipe (when omitted in code, it was auto-deduced if possible)
  FunctionPtr on_generic_func_call_instantiate(FunctionPtr generic_function, VertexAdaptor<op_func_call> call) {
    kphp_assert(generic_function->genericTs && call->reifiedTs);
    kphp_assert(generic_function->genericTs->size() == call->reifiedTs->size());

    // if f<T> is a member of an interface, instantiate f<T> in all derived classes
    // note, that we don't allow `class A { foo<T>() { ... } } class B : A { foo<T>() { ... } }` (moving a generic to self method)
    if (generic_function->modifiers.is_instance() && generic_function->is_virtual_method) {
      kphp_assert(generic_function->modifiers.is_abstract()); // should have been checked in advance
      for (ClassPtr derived : generic_function->class_id->get_all_derived_classes()) {
        if (derived != generic_function->class_id) {
          if (const auto* method = derived->members.get_instance_method(generic_function->local_name())) {
            on_generic_func_call_instantiate(method->function, call);
          }
        }
      }
    }

    std::string generated_name = call->reifiedTs->generate_instantiated_name(generic_function->name, generic_function->genericTs);
    FunctionPtr generated_function = instantiate_generic_function_concurrent(generic_function, call->reifiedTs, generated_name, function_stream);

    return generated_function;
  }

  // replace op_lambda with __construct call of a lambda class:
  // have `function() { ... }`
  // create a lambda class, which __invoke method calls this lambda (see lambda-utils.cpp)
  // return call of the __construct() method, it will replace op_lambda in AST
  VertexPtr on_op_lambda_convert_to_construct_lambda_class(VertexAdaptor<op_lambda> v_lambda) {
    FunctionPtr f_lambda = v_lambda->func_id;
    patch_lambda_function_add_uses_as_arguments(f_lambda);

    ClassPtr c_lambda = generate_lambda_class_wrapping_lambda_function(f_lambda);
    kphp_assert(f_lambda->class_id == c_lambda);

    if (v_lambda->lambda_class) { // while deducing types, we used this field to express inheritance from a typed callable
      kphp_assert(v_lambda->lambda_class->is_typed_callable_interface());
      inherit_lambda_class_from_typed_callable(c_lambda, v_lambda->lambda_class);
      G->require_function(v_lambda->lambda_class->get_instance_method("__invoke")->function, function_stream);
    }
    v_lambda->lambda_class = c_lambda;

    InstantiateGenericsAndLambdasPass pass_lambda(function_stream, forward_next_stream);
    run_function_pass(f_lambda, &pass_lambda);
    forward_next_stream << f_lambda; // nested lambdas were already processed by DeduceImplicitTypesAndCastsPass

    G->require_function(c_lambda->construct_function, function_stream);
    G->require_function(c_lambda->get_instance_method("__invoke")->function, function_stream);
    G->register_and_require_function(c_lambda->gen_holder_function(c_lambda->name), function_stream, true);

    return generate_lambda_class_construct_call_replacing_op_lambda(f_lambda, v_lambda);
  }

  // have `array_map(fn(...) => ..., $array)`
  // earlier, a lambda was wrapped to op_callback_of_builtin
  // we need to pass that wrapped lambda back to the pipeline to be handled, like it was used not in a built-in callback
  VertexPtr on_op_callback_of_builtin_patch_if_lambda(VertexAdaptor<op_callback_of_builtin> v_callback) {
    if (v_callback->func_id) {
      FunctionPtr f_lambda = v_callback->func_id;
      kphp_assert(f_lambda->is_lambda());
      patch_lambda_function_add_uses_as_arguments(f_lambda);

      InstantiateGenericsAndLambdasPass pass_lambda(function_stream, forward_next_stream);
      run_function_pass(f_lambda, &pass_lambda);
      forward_next_stream << f_lambda;
    }

    return v_callback;
  }
};

void InstantiateGenericsAndLambdasF::execute(FunctionPtr function, OStreamT& os) {
  auto& forward_next_stream = *os.project_to_nth_data_stream<0>();
  auto& newly_generated_functions_stream = *os.project_to_nth_data_stream<1>();

  InstantiateGenericsAndLambdasPass pass(newly_generated_functions_stream, forward_next_stream);
  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  forward_next_stream << function;
}
