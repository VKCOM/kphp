// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/instantiate-generics-and-lambdas.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/lambda-utils.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/pipes/clone-nested-lambdas.h"

/*
 * This pass creates new functions and passes them backwards to be handled again:
 * 1) it generates lambda classes for every op_lambda,
 *    and therefore pipes __construct and __invoke of that class
 * 2) it instantiates template functions knowing T1, etc
 *    and therefore pipes a newly-created function (an instantiated template function)
 */

// when we have a call `f<T>(...)`, we generate a new function `f$_$T`,
// which is not a template one, but an instantiated one
static FunctionPtr instantiate_template_function(FunctionPtr template_function,
                                                 GenericsInstantiationMixin *generics_instantiation,
                                                 const std::string &name_of_instantiated_function) {

  auto new_function = FunctionData::clone_from(template_function, name_of_instantiated_function);
  CloneNestedLambdasPass::run_if_lambdas_inside(new_function, nullptr);
  new_function->generics_declaration = nullptr;
  new_function->generics_instantiation = generics_instantiation;
  new_function->outer_function = template_function;

  // typically, `f(T $obj)` doesn't restrict T, in fact it means `f<T : any>(T $obj)`, T has extends_hint = nullptr
  // but `f(callable $c)` actually means `f<Callback1 : callable>(Callback1 $c)`, so Callback1 has extends_hint = callable
  // when real generics functions are done, we'll have a syntax to express `f<T : SomeInterface>(T $obj)`, for now we haven't
  for (const auto &generics_item : *template_function->generics_declaration) {
    const TypeHint *instantiation_type = generics_instantiation->find(generics_item.nameT);
    kphp_assert(instantiation_type);

    if (generics_item.extends_hint) {
      if (generics_item.extends_hint->try_as<TypeHintCallable>()) {   // prevent f(1) and other non-callable instantiations
        kphp_error(instantiation_type->try_as<TypeHintCallable>(),
                   fmt_format("Invalid template instantiation: expected {} to be {}, but {} found",
                              generics_item.nameT, TermStringFormat::paint_green(generics_item.extends_hint->as_human_readable()), TermStringFormat::paint_green(instantiation_type->as_human_readable())));
      }
    }
  }

  for (auto p : new_function->get_params()) {
    auto param = p.as<op_func_param>();
    if (param->type_hint) {
      param->type_hint = phpdoc_replace_genericsT_with_instantiation(param->type_hint, generics_instantiation);
    }
  }

  if (new_function->return_typehint && new_function->return_typehint->has_genericsT_inside()) {
    new_function->return_typehint = phpdoc_replace_genericsT_with_instantiation(new_function->return_typehint, generics_instantiation);
  }

  return new_function;
}

// the pass which is called from the F pipe
// (the F pipe is needed to have several outputs, which is impossible with Pass structure)
class InstantiateGenericsAndLambdasPass final : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream;

public:

  explicit InstantiateGenericsAndLambdasPass(DataStream<FunctionPtr> &function_stream)
    : function_stream(function_stream) {}

  std::string get_description() override {
    return "Instantiate template functions and lambdas";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (root->type() == op_lambda) {
      return on_op_lambda_convert_to_construct_lambda_class(root.as<op_lambda>());
    }
    if (root->type() == op_callback_of_builtin) {
      return on_op_callback_of_builtin_patch_if_lambda(root.as<op_callback_of_builtin>());
    }

    if (auto call = root.try_as<op_func_call>()) {
      if (call->func_id && call->func_id->is_template()) {
        call->func_id = on_template_func_call_instantiate(call->func_id, call);
      }
    }

    return root;
  }

  // have `f<T>(...)`, create f$_$T and return it
  // note, that we always know T due to the previous pipe (when omitted in code, it was auto-deduced if possible)
  FunctionPtr on_template_func_call_instantiate(FunctionPtr template_function, VertexAdaptor<op_func_call> call) {
    kphp_assert(template_function->is_template() && template_function->generics_declaration);
    kphp_assert(call->instantiation_list);
    kphp_assert(template_function->generics_declaration->size() == call->instantiation_list->size());

    // if f<T> is a member of an interface, instantate f<T> in all derived classes
    // note, that we don't allow `class A { foo<T>() { ... } } class B : A { foo<T>() { ... } }` (moving a template to self method)
    if (template_function->modifiers.is_instance() && template_function->is_virtual_method) {
      kphp_error(template_function->modifiers.is_abstract(),
                 fmt_format("{} is overridden in child classes. Overriding non-abstract instance template functions is unsupported", template_function->as_human_readable()));
      for (ClassPtr derived : template_function->class_id->get_all_derived_classes()) {
        if (derived != template_function->class_id) {
          if (const auto *method = derived->members.get_instance_method(template_function->local_name())) {
            on_template_func_call_instantiate(method->function, call);
          }
        }
      }
    }

    std::string generated_name = call->instantiation_list->generate_instantiated_func_name(template_function);
    FunctionPtr generated_function;

    G->operate_on_function_locking(generated_name, [&](FunctionPtr &f) {
      // a function with the same name could be already created earlier
      if (f) {
        generated_function = f;
        return;
      }

      f = instantiate_template_function(template_function, call->instantiation_list, generated_name);
      generated_function = f;

      if (template_function->modifiers.is_instance()) {
        AutoLocker<Lockable *> locker(&(*template_function->class_id));
        template_function->class_id->members.add_instance_method(generated_function);
      }

      kphp_assert(!generated_function->is_required);
      G->require_function(generated_function, function_stream);
    });

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

    if (v_lambda->lambda_class) {   // while deducing types, we used this field to express inheritance from a typed callable
      kphp_assert(v_lambda->lambda_class->is_typed_callable_interface());
      inherit_lambda_class_from_typed_callable(c_lambda, v_lambda->lambda_class);
    }
    v_lambda->lambda_class = c_lambda;

    function_stream << f_lambda;
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

      function_stream << f_lambda;
    }

    return v_callback;
  }
};


void InstantiateGenericsAndLambdasF::execute(FunctionPtr function, OStreamT &os) {
  auto &forward_next_stream = *os.project_to_nth_data_stream<0>();
  auto &newly_generated_functions_stream = *os.project_to_nth_data_stream<1>();

  InstantiateGenericsAndLambdasPass pass(newly_generated_functions_stream);
  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  forward_next_stream << function;
}
