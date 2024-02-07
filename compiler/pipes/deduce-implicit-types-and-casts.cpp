// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/deduce-implicit-types-and-casts.h"
#include "compiler/pipes/transform-to-smart-instanceof.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/generics-reification.h"
#include "compiler/vertex-util.h"
#include "compiler/lambda-utils.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"

/*
 * Deducing implicit types is a very important step of the compilation pipeline.
 * It does several related computations simultaneously.
 *
 * The idea behind this pass is based on the following.
 * If we have `f(callable(string):int $c)`, we need to have a step, when
 *    f(fn($s) => strlen($s))
 * will be treated as a typed callable (it means, that a lambda class should inherit an interface).
 * The question is: how to assign `fn($s)=>strlen($s)` to a _variable_ to make it also be treated as a typed callable?
 * The answer is: using phpdoc
 *    / ** @var callable(string):int * /
 *    $f = fn($s) => strlen($s);
 * Hence, there should be NO DIFFERENCE between assigning a variable and passing an argument.
 *
 * Expanding the idea, we realize, that
 *    f('strlen')
 * should be resolved with the very same implementation: treating `strlen` like a callable, not like a string.
 * When it comes to phpdoc,
 *    / ** @var callable * /
 *    $f_strlen = 'strlen';
 * should also work.
 * When it comes to a return value,
 *    / ** @return callable():int * /
 *    function f() { return fn() => 1; }        // 'return' should be also auto-casted
 * When it comes to class fields,
 *    / ** @var (callable(int):void)[] * /
 *    public $callbacks = [];
 *    $this->callbacks[] = function($int) {};   // assignments to fields and to sub-arrays are also auto-casted
 *
 * Moreover, if `f()` accepts an array of callables,
 *    f((callable():void)[] $cb_arr)
 * then every callable inside an array passed should be processed, auto-wrapped and auto-inherited:
 *    f([ function() {}, 'some_void_fn', [$obj, 'method'] ]);
 * Same for tuples, for example:
 *    / ** @var tuple(int, callable(int):int) * /
 *    $tup = tuple(0, 'treated_as_typed_callback');
 * This should work for arbitrary array dimensions and nesting.
 *
 * Typed callables are not only about type casting, they are also about providing implicit type hints:
 *    function f(callable(A):int $cb)
 * Then we auto-deduce that $a is A inside a lambda (no type hint!), use this deduction for assumptions and type cheking:
 *    f(fn($a) => $a->aMethod());
 *
 * `array_map()` and similar built-in functions are also covered by the same case,
 * with the only point, that we use ^2 and similar for deducing, instead of constant types:
 *    array_map(fn($a) => $a->aMethod(), [new A]);
 *
 * In the future, when vectors and maps are implemented, we would want this to work:
 *    / ** @var vector<int> * /
 *    $vec = vector();     // <int> is auto-deduced due to phpdoc
 * Same for passing vectors as arguments, returning them, same for vectors inside arrays, etc.
 *
 * In the future, we want to auto-cast arrays to tuples in the same cases:
 *    / ** @return tuple(int, string) * /
 *    function f() { return [1, 's']; }     // replaced with { return tuple(1, 's'); }
 * That is specially useful for shapes:
 *    / ** @param shape(x: int) $arg * /
 *    function f($arg) { ... }
 * It would be allowed to call `f([...])`, not `f(shape([...]))`:
 *    f(['x' => 1]);      // replaced with f(shape(['x' => 1]))
 * That could even work for class fields, giving an ability to use tuples as default values:
 *    / ** @var tuple(int[], string) * /
 *    public $tup = [[], ''];
 *
 * As a result, we have
 * - an lhs type — callable / typed callable / vector / tuple / etc.
 * - an actual argument (rhs)
 * What we do, is patching rhs to fit lhs type.
 * That's the thing, that `patch_rhs_casting_to_lhs_type()` pretty much does.
 * If lhs type is `int`, or `A[]`, or other types that surely don't patch rhs any way,
 * we don't have to process lhs type deeply searching for callables/tuples.
 * A bit flag `TypeHint::flag_potentially_casts_rhs` is a moment of this optimization: it indicates the necessity of this recursion.
 *
 * ---
 *
 * Now imagine, that we are calling not `f('strlen')`, but
 *    $obj->f('strlen');
 * We need to do the same transformations as above — BUT we have to resolve an ->arrowCall() beforehand.
 * Hence, we need to calculate an assumption for $obj — at the moment we need it.
 *
 * We need assumptions not only for ->arrowCalls(), but also for generics instantiations:
 *    function f<T>(T $obj, callable(T):void) { ... }
 * Having a call in terms of generic functions,
 *    f($obj, function($o) { ... });
 * We need an assumption for $obj, to instantiate f(), to auto-infer $o as an argument.
 *
 * As a result, this step simultaneously calculates all assumptions on demand.
 * Not only for ->arrowCalls(), but for ->fields and other cases – everything except binding ()-invocations, that's resolved later.
 *
 * Here we also bind func calls (set func_id of every op_func_call). It seems logical to do this while resolving assumptions.
 * func_id becomes valid for functions and methods, but for generics, it still points to a generic function.
 * (generics are instantiated a bit later, and func_id is reassigned to an instantiation)
 *
 * Note, that assumptions are calculated on demand — and only up to the point we first need it:
 *    $a = new A;
 *    $a->f();     // we need an assumption
 *    $a = new B:  // this goes after
 * When an assumption for $a is calculated, stop_at is ->f() invocation, so an assumption is fixed as A.
 * `new B` conflicts with `new A`, this conflict is found later, resulting in an error "was already assumed A".
 * If we add a phpdoc in advance,
 *    / ** @var $a SomeClass * /
 * then it has more priority then code-based assumptions, `new A` and `new B` are not analyzed at all.
 *
 * ---
 *
 * A function can reach this pass not only via compilation pipeline, like any other pass.
 * This pass is also executed for `f` if we need to calc assuption for return of `f`, see `assume_return_of_function()`.
 * That's why it's guarded with multithreading once lock, see `check_function()` below.
 *
 * ---
 *
 * Concluding, this step does the following, simultaneously:
 * - resolves func calls and binds func_id to every op_func_call
 * - modifies arguments of functions or rhs at assignments to fit phpdoc (lhs)
 * - when an ->arrowCall() found, calculates assumptions to bind func_id
 * - when an ->field access found, calculates assumptions to bind var_id
 * - calculates assumptions for some other needs for later passes
 * - deduces types of lambdas arguments passed to array_map() or similar, as well as to typed callables
 * - deduces <T> for generic calls, saving it to call->reifiedTs
 * - some other minors, read the code
 */


// having a lambda with assumed types, construct TypeHintCallable from it
// (to inherit from a typed callable interface, for example)
static InterfacePtr get_typed_callable_interface_from_lambda(FunctionPtr f_lambda) {
  std::vector<const TypeHint *> arg_types;
  arg_types.reserve(f_lambda->get_params().size());

  for (auto p : f_lambda->get_params()) {
    const TypeHint *param_hint = p.as<op_func_param>()->type_hint;
    if (!param_hint) {
      return InterfacePtr{};
    }
    arg_types.emplace_back(param_hint);
  }
  if (!f_lambda->return_typehint) {
    return InterfacePtr{};
  }

  const TypeHint *as_callable = TypeHintCallable::create(std::move(arg_types), f_lambda->return_typehint);
  return as_callable->is_typedata_constexpr() ? as_callable->try_as<TypeHintCallable>()->get_interface() : InterfacePtr{};
}

// having an array_map type_hint `callable(^2[*] $x)` and a call `array_map(fn($a) => ..., [new A]),
// deduce that $a is A, based on ^arg refs and assumptions
static const TypeHint *patch_type_hint_with_argref_in_context_of_call(const TypeHint *type_hint, FunctionPtr current_function, VertexAdaptor<op_func_call> call) {
  const TypeHint *replaced = type_hint->replace_children_custom([current_function, call](const TypeHint *child) {
    if (const auto *as_arg_ref = child->try_as<TypeHintArgRef>()) {
      if (auto call_arg = VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, call)) {
        return assume_class_of_expr(current_function, call_arg, call).assum_hint ?: child;
      }
    }
    if (const auto *as_arg_ref_subkey = child->try_as<TypeHintArgSubkeyGet>()) {
      const TypeHint *inner = patch_type_hint_with_argref_in_context_of_call(as_arg_ref_subkey->inner, current_function, call) ?: as_arg_ref_subkey->inner;
      if (const auto *as_array = inner->unwrap_optional()->try_as<TypeHintArray>()) {
        return as_array->inner;
      }
    }
    return child;
  });

  return replaced->has_argref_inside() ? nullptr : replaced;
}

// take a look at `function map<TIn, TOut>(TIn[] $in, callable(TIn):TOut $callback)`
// it's a generic function, TIn can be reified as an argument, but TOut = "what a lambda will return"
// having a call `map([new A], fn($a) => $a)`, at reification scan KPHP deduces `TIn = A, TOut = unknown`
// then it starts patching arguments, `fn` becomes `callable(A):TOut`
// this function fills TOut=(assumption of fn)=A, which is also set to f_lambda->return_typehint
static const TypeHint *patch_lambda_return_hint_replacing_genericT_in_call(const TypeHint *type_hint, FunctionPtr f_lambda, VertexAdaptor<op_func_call> call) {
  if (const auto *as_genericT = type_hint->try_as<TypeHintGenericT>()) {
    // implementation detail: this will execute DeduceImplicitTypesAndCastsPass of f_lambda in current thread
    // like any assumptions, it will correctly analyze `{ $a2 = $a; return $a2; }`
    // note, that `map([1], fn($i) => $i)` will still work, because assumption `$i=int` is saved
    // (unlike regular functions, assumptions for lambda params are saved even for primitives)
    if (Assumption ret_assum = assume_return_of_function(f_lambda)) {
      call->reifiedTs->provideT(as_genericT->nameT, ret_assum.assum_hint, call);
      return ret_assum.assum_hint;
    }
  }

  return type_hint;
}

// having an rhs (fn(), or 'strlen', or [$obj, 'method']) passed as typed/untyped callable,
// perform necessary modifications of rhs
// the argument `call` is valid for func calls (to resolve ^2 and fill TOut), but is empty for assignments
// mind that lambdas and non-lambdas passed to built-in functions are represented as op_callback_of_builtin
void patch_rhs_casting_to_callable(VertexPtr &rhs, const TypeHintCallable *as_callable, VertexAdaptor<op_func_call> call) {
  bool is_extern_func_param = call && call->func_id && call->func_id->is_extern();
  InterfacePtr typed_interface = as_callable->is_typed_callable() && as_callable->is_typedata_constexpr() ? as_callable->get_interface() : InterfacePtr{};

  if (as_callable->is_typed_callable() && rhs->type() == op_lambda) {
    // function(...) { ... } passed as callable(int):string (to array_map in particular)
    FunctionPtr f_lambda = rhs.as<op_lambda>()->func_id;
    auto f_lambda_params = f_lambda->get_params();
    for (int i = 0; i < f_lambda_params.size() && i < as_callable->arg_types.size(); ++i) {
      auto l_param = f_lambda_params[i].as<op_func_param>();
      if (!l_param->type_hint) {
        l_param->type_hint = as_callable->arg_types[i];
      }
      if (l_param->type_hint->has_argref_inside()) {
        l_param->type_hint = patch_type_hint_with_argref_in_context_of_call(l_param->type_hint, stage::get_function(), call);
      }
    }
    if (!f_lambda->return_typehint) {
      f_lambda->return_typehint = as_callable->return_type;
    }
    if (as_callable->return_type->has_genericT_inside()) {
      f_lambda->return_typehint = patch_lambda_return_hint_replacing_genericT_in_call(as_callable->return_type, f_lambda, call);
      typed_interface = get_typed_callable_interface_from_lambda(f_lambda);
    }
    rhs.as<op_lambda>()->lambda_class = typed_interface;  // save for the next pass

  } else if (VertexUtil::unwrap_inlined_define(rhs)->type() == op_string) {
    rhs = VertexUtil::unwrap_inlined_define(rhs);
    // 'strlen' or 'A::staticMethod'
    std::string func_name = replace_characters(rhs->get_string(), ':', '$');
    func_name = replace_backslashes(func_name[0] == '\\' ? func_name.substr(1) : func_name);

    if (is_extern_func_param) {
      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create().set_location(rhs);
      v_callback->str_val = func_name;
      rhs = v_callback;
    } else {
      auto v_lambda = VertexAdaptor<op_lambda>::create().set_location(rhs);
      v_lambda->func_id = generate_lambda_from_string_or_array(stage::get_function(), func_name, VertexPtr{}, rhs);
      v_lambda->lambda_class = typed_interface;
      rhs = v_lambda;
    }
  } else if (rhs->type() == op_array && rhs->size() == 2 && VertexUtil::unwrap_inlined_define(rhs->front())->type() == op_string
             && VertexUtil::unwrap_inlined_define(rhs->back())->type() == op_string) {
    // ['A', 'staticMethod']
    auto unwrapped_front = VertexUtil::unwrap_inlined_define(rhs->front());
    auto unwrapped_back = VertexUtil::unwrap_inlined_define(rhs->back());

    std::string func_name = unwrapped_front->get_string() + "$$" + unwrapped_back->get_string();
    func_name = replace_backslashes(func_name[0] == '\\' ? func_name.substr(1) : func_name);

    if (is_extern_func_param) {
      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create().set_location(rhs);
      v_callback->str_val = func_name;
      rhs = v_callback;
    } else {
      auto v_lambda = VertexAdaptor<op_lambda>::create().set_location(rhs);
      v_lambda->func_id = generate_lambda_from_string_or_array(stage::get_function(), func_name, VertexPtr{}, rhs);
      v_lambda->lambda_class = typed_interface;
      rhs = v_lambda;
    }

  } else if (rhs->type() == op_array && rhs->size() == 2 && VertexUtil::unwrap_inlined_define(rhs->back())->type() == op_string) {
    // [$obj, 'method'] or [getObject(), 'method'] or similar
    // pay attention, that $obj (rhs->front()) is captured in an auto-created wrapping lambda or in a c++ lambda in case of extern
    auto unwrapped_back = VertexUtil::unwrap_inlined_define(rhs->back());

    if (is_extern_func_param) {
      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create(rhs->front()).set_location(rhs);
      v_callback->str_val = unwrapped_back->get_string();
      rhs = v_callback;
    } else {
      auto v_lambda = VertexAdaptor<op_lambda>::create().set_location(rhs);
      v_lambda->func_id = generate_lambda_from_string_or_array(stage::get_function(), unwrapped_back->get_string(), rhs->front(), rhs);
      v_lambda->lambda_class = typed_interface;
      rhs = v_lambda;
    }
  }

  if (is_extern_func_param && rhs->type() != op_callback_of_builtin) {
    if (rhs->type() == op_lambda) {
      // array_map(fn(), ...)
      FunctionPtr f_lambda = rhs.as<op_lambda>()->func_id;
      std::vector<VertexPtr> cpp_captured_list;
      for (auto var_as_use : f_lambda->uses_list) {
        cpp_captured_list.emplace_back(var_as_use.clone());
      }

      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create(cpp_captured_list);
      v_callback->func_id = f_lambda;
      v_callback->str_val = f_lambda->name;
      rhs = v_callback;
    } else if (rhs->type() == op_null && FFIRoot::is_ffi_scope_call(call)) {
      // allow null as callback argument for FFI functions
    } else {
      // array_map($f, ...) or array_map(getCallback(), ...) or similar
      auto bound_this = rhs;
      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create(bound_this).set_location(rhs);
      v_callback->str_val = "__invoke";
      rhs = v_callback;
    }
  }
}

// the main function that patches rhs to fit lhs type, see detailed comments above
// called when rhs is passed as @param, or when @var $v = rhs, or for "return expr" to fit @return, etc
static void patch_rhs_casting_to_lhs_type(VertexPtr &rhs, const TypeHint *lhs_type_hint, VertexAdaptor<op_func_call> call = {}) {
  // to avoid redundant work, every type hint has a special flag whether it potentially can require an rhs cast
  // for example, we do nothing for @param int or @param string[], but start analyzing rhs for @param callable
  if (!lhs_type_hint->has_flag_maybe_casts_rhs()) {
    return;
  }

  // return cond ? ... : ... — cast both ternary expressions
  if (rhs->type() == op_ternary) {
    patch_rhs_casting_to_lhs_type(rhs.as<op_ternary>()->false_expr(), lhs_type_hint);
    patch_rhs_casting_to_lhs_type(rhs.as<op_ternary>()->true_expr(), lhs_type_hint);
    return;
  }

  // leave T from ?T
  lhs_type_hint = lhs_type_hint->unwrap_optional();

  // @param callable — extracted to a separate method, not to mess here
  if (const auto *as_callable = lhs_type_hint->try_as<TypeHintCallable>()) {
    patch_rhs_casting_to_callable(rhs, as_callable, call);
  }

  // @param (callable():void)[], cast every rhs array's element
  if (const auto *as_array = lhs_type_hint->try_as<TypeHintArray>()) {
    if (auto rhs_as_array = rhs.try_as<op_array>()) {
      for (auto &item : *rhs_as_array) {
        if (auto as_double_arrow = item.try_as<op_double_arrow>()) {
          patch_rhs_casting_to_lhs_type(as_double_arrow->value(), as_array->inner, call);
        } else {
          patch_rhs_casting_to_lhs_type(item, as_array->inner, call);
        }
      }
    }
  }

  // @param tuple(int, callable(...):void), cast correspondent arguments of a passed tuple
  if (const auto *as_tuple = lhs_type_hint->try_as<TypeHintTuple>()) {
    if (auto rhs_as_tuple = rhs.try_as<op_tuple>()) {
      for (int i = 0; i < as_tuple->items.size() && i < rhs_as_tuple->size(); ++i) {
        patch_rhs_casting_to_lhs_type(rhs_as_tuple->args()[i], as_tuple->items[i], call);
      }
    }
  }

  // @param shape(cb: callable(...):int), cast corresponding arguments of a passed shape
  if (const auto *as_shape = lhs_type_hint->try_as<TypeHintShape>()) {
    if (auto rhs_as_shape = rhs.try_as<op_shape>()) {
      for (auto sub_expr : rhs_as_shape->args()) {
        const std::string &shape_key = VertexUtil::get_actual_value(sub_expr->front())->get_string();
        if (const TypeHint *lhs_type_at_key = as_shape->find_at(shape_key)) {
          patch_rhs_casting_to_lhs_type(sub_expr->back(), lhs_type_at_key, call);
        }
      }
    }
  }
}


// we have @kphp-infer cast and ::: syntax in functions.txt that means auto-casting of passed arguments,
// but we don't auto-cast in files marked with @kphp-strict-types-enable
static bool is_implicit_cast_allowed(bool strict_types, const TypeHint *type_hint_cast_to) {
  if (!strict_types) {
    return true;
  }
  // converting to regexp is OK even in strict_types mode
  if (const auto *primitive_type = type_hint_cast_to->try_as<TypeHintPrimitive>()) {
    return primitive_type->ptype == tp_regexp;
  }
  return false;
}

// we have @kphp-infer cast and ::: syntax in functions.txt that means auto-casting of passed arguments
// this function performs this cast: it takes rhs and returns a nesessary op_conv_* wrapping rhs
static VertexPtr implicit_cast_call_arg_to_cast_param(VertexPtr rhs, const TypeHint *type_hint_cast_to, bool ref_flag) {
  PrimitiveType tp = tp_any;
  if (type_hint_cast_to->try_as<TypeHintArray>()) {
    tp = tp_array;
  } else if (const auto *as_primitive = type_hint_cast_to->try_as<TypeHintPrimitive>()) {
    tp = as_primitive->ptype;
  }

  if (ref_flag) {
    switch (tp) {
      case tp_array:
        return VertexUtil::create_conv_to_lval(tp_array, rhs);
      case tp_int:
        return VertexUtil::create_conv_to_lval(tp_int, rhs);
      case tp_string:
        return VertexUtil::create_conv_to_lval(tp_string, rhs);
      case tp_mixed:
        return rhs;
      default:
        kphp_error (0, "Too hard rule for cast a param with ref_flag");
        return rhs;
    }
  }
  switch (tp) {
    case tp_int:
      return VertexUtil::create_conv_to(tp_int, rhs);
    case tp_bool:
      return VertexUtil::create_conv_to(tp_bool, rhs);
    case tp_string:
      return VertexUtil::create_conv_to(tp_string, rhs);
    case tp_float:
      return VertexUtil::create_conv_to(tp_float, rhs);
    case tp_array:
      return VertexUtil::create_conv_to(tp_array, rhs);
    case tp_regexp:
      return VertexUtil::create_conv_to(tp_regexp, rhs);
    case tp_mixed:
      return VertexUtil::create_conv_to(tp_mixed, rhs);
    default:
      return rhs;
  }
}


// this pass is very tricky, probably it's the most cognitively hard in KPHP
// a function can reach it in 3 ways actually:
// - normally, through compilation pipeline
// - in on_finish(), this pass is executed for all nested lambdas
// - when f1 needs to know assumption for return for f2, this pass is launched for f2
// that's why we add a multithreading guard here, to ensure it's executed only the first time requested
bool DeduceImplicitTypesAndCastsPass::check_function(FunctionPtr f) const {
  auto expected = FunctionData::AssumptionStatus::uninitialized;
  if (f->assumption_pass_status.compare_exchange_strong(expected, FunctionData::AssumptionStatus::processing_deduce_pass)) {
    f->assumption_processing_thread = std::this_thread::get_id();
    return true;
  } else if (expected == FunctionData::AssumptionStatus::processing_deduce_pass) {
    while (f->assumption_pass_status == FunctionData::AssumptionStatus::processing_deduce_pass && f->assumption_processing_thread != std::this_thread::get_id()) {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }
  }
  return false;
}

void DeduceImplicitTypesAndCastsPass::on_start() {
  FunctionPtr f = current_function;
  kphp_assert(f->assumption_pass_status == FunctionData::AssumptionStatus::processing_deduce_pass && !f->is_generic());

  // this is a hack, but we can't insert this pass before DeduceImplicitTypesAndCastsPass in compilation pipeline
  // hence, we manually call it from here
  // some time later, contents of "smart instanceof" pass has to be reconsidered and merged into this pass,
  // because logically instanceof specifics is also about implicit casts, actually
  TransformToSmartInstanceofPass pass;
  run_function_pass(f, &pass);
  stage::set_name(get_description());
}

void DeduceImplicitTypesAndCastsPass::on_finish() {
  current_function->assumption_pass_status = FunctionData::AssumptionStatus::done_deduce_pass;
  current_function->assumption_processing_thread = std::thread::id{};

  // while reifying genericTs for calls, not every call could fill all Ts in-place,
  // some or them can be reified only when traversing the tree up, in on_exit_vertex of a parent
  // I have an idea of how to rewrite in a more elegant way with user_recursion, but it will complicate this MR for now
  generic_calls.reverse();
  for (auto call : generic_calls) {
    stage::set_location(call->location);
    FunctionPtr f_called = call->func_id;
    bool old_syntax = !GenericsDeclarationMixin::is_new_kphp_generic_syntax(f_called->phpdoc);
    check_reifiedTs_for_generic_func_call(f_called->genericTs, call, old_syntax);
  }

  if (stage::has_error()) {
    return;
  }

  for (FunctionPtr f_lambda : nested_lambdas) {
    DeduceImplicitTypesAndCastsPass pass_lambda;
    run_function_pass(f_lambda, &pass_lambda);
  }
}

// analyze a vertex in the pass
// on_exit_vertex (not on enter) is reasonable
VertexPtr DeduceImplicitTypesAndCastsPass::on_exit_vertex(VertexPtr root) {
  stage::set_location(root->location);

  if (root->type() == op_set) {
    auto lhs = root.as<op_set>()->lhs();
    auto &rhs = root.as<op_set>()->rhs();
    if (lhs->type() == op_var) {
      on_set_to_var(lhs.as<op_var>(), rhs);
    } else if (lhs->type() == op_instance_prop) {
      on_set_to_instance_prop(lhs.as<op_instance_prop>(), rhs);
    } else if (lhs->type() == op_index) {
      on_set_to_index(lhs, rhs);
    }
  } else if (root->type() == op_func_call) {
    on_func_call(root.as<op_func_call>());
  } else if (root->type() == op_func_param) {
    on_func_param(root.as<op_func_param>());
  } else if (root->type() == op_return) {
    on_return(root.as<op_return>());
  } else if (root->type() == op_phpdoc_var) {
    on_phpdoc_for_var(root.as<op_phpdoc_var>());
  } else if (root->type() == op_list) {
    on_list(root.as<op_list>());
  } else if (root.try_as<op_instance_prop>()) {
    on_instance_prop(root.as<op_instance_prop>());
  } else if (root->type() == op_clone) {
    on_clone(root.as<op_clone>());
  } else if (root->type() == op_throw) {
    on_throw(root.as<op_throw>());
  } else if (root->type() == op_lambda) {
    on_lambda(root.as<op_lambda>());
  }

  return root;
}

// at first, for every function with `@param A` or other instances, add assumptions
// such arguments won't be splited (see CFG::split_var), so they can't be reassigned
// note, that for `@param string` and other primitives we allow splitting: they don't contain instances, and therefore not saved
void DeduceImplicitTypesAndCastsPass::on_func_param(VertexAdaptor<op_func_param> v_param) {
  if (v_param->type_hint) {
    assumption_add_for_var(current_function, v_param->var()->str_val, Assumption(v_param->type_hint), v_param);
  }
}

// when we meet @var {type} $v,
// 1) for primitives (strings, etc — no instances == no assumptions) we allow splitting by cfg, skip them
// 2) for non-primitives, we check that phpdocs coincide with each other and with code-based assumptions (if any made before phpdoc occured)
void DeduceImplicitTypesAndCastsPass::on_phpdoc_for_var(VertexAdaptor<op_phpdoc_var> v_phpdoc) {
  Assumption existing = current_function->get_assumption_for_var(v_phpdoc->var()->str_val);
  const std::string &var_name = v_phpdoc->var()->str_val;

  if (existing) {
    const TypeHint *prev_phpdoc_hint = get_found_phpdoc_for_var(var_name);
    if (prev_phpdoc_hint) {
      // we had @var already, so we expect this @var to be exactly the same
      kphp_error_return(prev_phpdoc_hint == v_phpdoc->type_hint,
                        fmt_format("${} has inconsistent phpdocs.\nAt first it was declared as @var {}, and then @var {}",
                                   var_name, TermStringFormat::paint_green(prev_phpdoc_hint->as_human_readable()), TermStringFormat::paint_green(v_phpdoc->type_hint->as_human_readable())));
    } else {
      // we had an assumption for $v already ($v was used to bind arrow calls or instantiate a generic), check that phpdoc equals
      kphp_error_return(existing.assum_hint == v_phpdoc->type_hint,
                        fmt_format("You want ${} to have the type {}, but ${} was already used above in this function.\n${} was already assumed to be {}.\nMove this phpdoc above all assignments to ${} to resolve this conflict.",
                                   var_name, TermStringFormat::paint_green(v_phpdoc->type_hint->as_human_readable()), var_name, var_name, TermStringFormat::paint_green(existing.assum_hint->as_human_readable()), var_name));
    }
  } else {
    // it it's a primitive, it would be skipped; if not — saved as an assumption
    assumption_add_for_var(current_function, var_name, Assumption(v_phpdoc->type_hint), v_phpdoc);
  }

  phpdocs_for_vars.emplace_front(v_phpdoc);
}

// for every `f(...)`, bind func_id
// for every call argument, patch it to fit @param of f()
// if f() is a generic function `f<T1,T2,...>(...)`, deduce instantiation Ts (save call->reifiedTs)
void DeduceImplicitTypesAndCastsPass::on_func_call(VertexAdaptor<op_func_call> call) {
  if (!call->func_id) {
    if (call->extra_type == op_ex_constructor_call) {
      // 'new XXX(...)', that was transformed into __construct(op_alloc XXX, ...)
      auto lhs = call->args()[0].as<op_alloc>();
      ClassPtr klass = lhs->allocated_class;

      kphp_error_return(klass, fmt_format("Class {} not found", resolve_uses(current_function, lhs->allocated_class_name)));
      kphp_error_return(!klass->is_trait(), fmt_format("Can't instantiate {}, it's a trait", klass->name));
      kphp_error_return(!klass->is_interface(), fmt_format("Can't instantiate {}, it's an interface", klass->name));
      kphp_error_return(!klass->modifiers.is_abstract(), fmt_format("Can't instantiate {}, it's an abstract class", klass->name));
      kphp_assert(klass->construct_function);

      call->func_id = klass->construct_function;

    } else if (call->extra_type == op_ex_func_call_arrow) {
      // lhs->method(...), that was transformed into method(lhs, ...): we need to infer the class of lhs
      auto lhs = call->args()[0];

      ClassPtr klass = resolve_class_of_arrow_access(current_function, lhs, call);
      if (!klass) {   // if so, an error has already been printed
        return;
      }
      const auto *method = klass->get_instance_method(call->str_val);
      if (!method) {
        kphp_error_return(!klass->members.get_static_method(call->str_val), fmt_format("Static method {} is called using $this", call->str_val));
        kphp_error_return(0, fmt_format("Method {}() not found in {} {}", call->str_val, klass->is_interface() ? "interface" : "class", klass->as_human_readable()));
      }

      call->func_id = method->function;

    } else {
      // just a regular function call in a global namespace
      call->func_id = G->get_function(call->str_val);
      if (!call->func_id) {
        print_error_unexisting_function(call->str_val);
        return;
      }
    }
  }

  FunctionPtr f_called = call->func_id;
  auto call_args = call->args();
  auto f_called_params = f_called->get_params();

  // if we are calling `f<T>`, then `f` has not been instantiated yet at this point, so we have a generic func call
  // at first, we need to know all generic types (call->reifiedTs)
  // 1) they could be explicitly set using syntax `f/*<T1, T2>*/(...)`
  // 2) they could be omitted and should be auto-reified
  if (f_called->is_generic()) {
    bool old_syntax = !GenericsDeclarationMixin::is_new_kphp_generic_syntax(f_called->phpdoc);

    // if `f` is a variadic generic `f<...TArg>` called with N=2 arguments, `f$n2<TArg1, TArg2>` is created and called instead
    // (same for manually provided variadic types: `f/*<int, string, ?A>*/` => `f$n3<int, string, ?A>`)
    if (f_called->has_variadic_param && f_called->genericTs->is_variadic()) {
      int n_variadic = call->reifiedTs
                       ? call->reifiedTs->commentTs->size()
                       : call_args.size() - (f_called_params.size() - 1 + f_called->has_implicit_this_arg());
      if (n_variadic >= 0) {
        f_called = convert_variadic_generic_function_accepting_N_args(f_called, n_variadic);
        f_called_params = f_called->get_params();
        call->func_id = f_called;
      }
    }

    if (call->reifiedTs) {
      kphp_assert(call->reifiedTs->empty() && call->reifiedTs->commentTs);
      apply_instantiationTs_from_php_comment(f_called, call, call->reifiedTs->commentTs);
    } else {
      reify_function_genericTs_on_generic_func_call(current_function, f_called, call, old_syntax);
    }
    // we can't check call->reifiedTs, because some of them may be pushed when exiting a parent vertex
    // see on_finish()
    generic_calls.emplace_front(call);
  }

  // now, loop through every argument and potentially patch it
  for (int i = 0; i < f_called_params.size() && i < call_args.size(); ++i) {
    auto param = f_called_params[i].as<op_func_param>();

    if (param->type_hint) {
      patch_call_arg_on_func_call(param, call_args[i], call);
      if (param->extra_type == op_ex_param_variadic) {    // all the rest arguments are meant to be passed to this param
        for (++i; i < call_args.size(); ++i) {            // here, they are not replaced with an array: see CheckFuncCallsAndVarargPass
          patch_call_arg_on_func_call(param, call_args[i], call);
        }
      }
    }
  }
}

// a helper to print a human-readable error for `f()` when f not found
// the return value is not used, it's just to make the code shorter by using `return kphp_error(...)`
int DeduceImplicitTypesAndCastsPass::print_error_unexisting_function(const std::string &call_string) {
  std::string unexisting_func_name = replace_call_string_to_readable(call_string);

  // people try to use some of these and struggle to find an alternative
  if (unexisting_func_name == "reset") {
    return kphp_error(0, "KPHP does not support reset(), maybe array_first_value() is what you are looking for?");
  }
  if (unexisting_func_name == "end") {
    return kphp_error(0, "KPHP does not support end(), maybe array_last_value() is what you are looking for?");
  }

  int pos1 = unexisting_func_name.find("::");
  int pos2 = unexisting_func_name.rfind("::");
  if (pos1 != std::string::npos) {
    std::string class_name = unexisting_func_name.substr(0, pos1);
    if (!G->get_class(class_name)) {
      return kphp_error(0, fmt_format("Unknown class {}, can't call its method", class_name));
    }
    if (pos1 != pos2) {
      return kphp_error(0, fmt_format("Unknown method {}", unexisting_func_name.substr(0, pos2)));
    }
    return kphp_error(0, fmt_format("Unknown method {}", unexisting_func_name));
  }

  return kphp_error(0, fmt_format("Unknown function {}", unexisting_func_name));
}

// having `f($arg1, $arg2)`, this functions patches $arg_i to fit the corresponding @param of f()
void DeduceImplicitTypesAndCastsPass::patch_call_arg_on_func_call(VertexAdaptor<op_func_param> param, VertexPtr &call_arg, VertexAdaptor<op_func_call> call) {
  const TypeHint *param_hint = param->type_hint;

  // for cast params (::: in functions.txt or '@kphp-infer cast') we add conversions automatically (implicit casts),
  // unless the file from where we're calling this function is annotated with strict_types=1
  if (param->is_cast_param && is_implicit_cast_allowed(current_function->file_id->is_strict_types, param_hint) && call_arg->type() != op_varg) {
    call_arg = implicit_cast_call_arg_to_cast_param(call_arg, param_hint, param->var()->ref_flag);
    return;
  }

  // at this pass, generics are not instantiated yet,
  // but call->reifiedTs is already filled (at least, partially)
  // so, if we need to cast to `callable(T)`, we already know T
  if (param_hint->has_genericT_inside()) {
    kphp_assert(call->reifiedTs);
    param_hint = phpdoc_replace_genericTs_with_reified(param_hint, call->reifiedTs);
  }

  // now, patch call_arg to fit param_hint
  // example: an op_lambda passed to a typed callable @param is auto-inherited from a typed interface
  // example: a string passed to a callable is converted to op_callback_of_builtin
  if (param->extra_type != op_ex_param_variadic) {
    patch_rhs_casting_to_lhs_type(call_arg, param_hint, call);
  } else if (param->type_hint->try_as<TypeHintArray>()) {
    patch_rhs_casting_to_lhs_type(call_arg, param_hint->try_as<TypeHintArray>()->inner, call);
  }
}

// for `return ...`, patch the return expression to fit @return
void DeduceImplicitTypesAndCastsPass::on_return(VertexAdaptor<op_return> v_return) {
  if (current_function->return_typehint && v_return->has_expr()) {
    patch_rhs_casting_to_lhs_type(v_return->expr(), current_function->return_typehint);
  }

  // this allows mixing void and non-void, but probably, somewhen we'll get rid of this in vkcom
  if (current_function->type == FunctionData::func_main ||
      current_function->type == FunctionData::func_switch ||
      current_function->disabled_warnings.count("return")) {
    if (v_return->has_expr() && v_return->expr()->type() != op_null) {
      v_return->expr() = VertexAdaptor<op_force_mixed>::create(v_return->expr());
    }
  }
}

// handle `$v = rhs`, patch rhs to fit @var if it was set to $v
void DeduceImplicitTypesAndCastsPass::on_set_to_var(VertexAdaptor<op_var> lhs, VertexPtr &rhs) {
  const std::string &var_name = lhs->str_val;
  const TypeHint *var_type_hint = get_found_phpdoc_for_var(var_name);
  if (var_type_hint) {
    patch_rhs_casting_to_lhs_type(rhs, var_type_hint);
  }
  if (!var_type_hint && current_function->get_assumption_for_var(var_name)) {
    // now we deal with such situation: "$a = new A; $a->method(); $a = new B; (here)"
    // when there are no phpdocs, an assumption A was calculated for ->method(), and on "$a = new B" we check it remains
    // note, that @var phpdocs have more priority: we don't calculate assumptions manually if it exists
    // this check doesn't save us from all issues, but handling it here and in op_list (below) covers 95% use cases
    Assumption rhs_assum = assume_class_of_expr(current_function, rhs, rhs);
    assumption_add_for_var(current_function, var_name, rhs_assum, lhs);
  }
}

// handle `$object->field = rhs`, patch rhs to fir @var of the field
void DeduceImplicitTypesAndCastsPass::on_set_to_instance_prop(VertexAdaptor<op_instance_prop> lhs, VertexPtr &rhs) {
  const auto *field = lhs->class_id ? lhs->class_id->get_instance_field(lhs->str_val) : nullptr;
  if (field && field->type_hint) {
    patch_rhs_casting_to_lhs_type(rhs, field->type_hint);
  }
}

// handle `$a[$idx][$idx2] = rhs` or `$a[] = rhs`, patch rhs to fit @var if it was set to $a
void DeduceImplicitTypesAndCastsPass::on_set_to_index(VertexPtr lhs, VertexPtr &rhs) {
  int depth = 0;
  while (lhs->type() == op_index) {
    depth++;
    lhs = lhs.as<op_index>()->array();
  }
  if (lhs->type() == op_var) {
    const TypeHint *type_hint_at_depth = get_found_phpdoc_for_var(lhs.as<op_var>()->str_val);
    while (type_hint_at_depth && depth--) {
      const auto *as_array = type_hint_at_depth->unwrap_optional()->try_as<TypeHintArray>();
      type_hint_at_depth = as_array ? as_array->inner : nullptr;
    }
    if (type_hint_at_depth) {
      patch_rhs_casting_to_lhs_type(rhs, type_hint_at_depth);
    }
  }
}

// handle `list($a, $b) = ...`, patch rhs by indexes to fit @var if declared above
void DeduceImplicitTypesAndCastsPass::on_list(VertexAdaptor<op_list> v_list) {
  // example: `list($v) = [fn($v) => $v=1];`, $v was declared as @var typed callable above
  if (vk::any_of_equal(v_list->array()->type(), op_array, op_tuple)) {
    for (auto x : v_list->list()) {
      auto kv = x.as<op_list_keyval>();
      if (kv->key()->type() == op_int_const && kv->var()->type() == op_var) {
        int ith_index = parse_int_from_string(kv->key().as<op_int_const>());
        const TypeHint *var_type_hint = get_found_phpdoc_for_var(kv->var()->get_string());
        if (ith_index >= 0 && ith_index < v_list->array()->size() && var_type_hint) {
          patch_rhs_casting_to_lhs_type(v_list->array().as<meta_op_varg>()->args()[ith_index], var_type_hint);
        }
      }
    }
  }

  // handle "$a = new A; $a->method(); list($a) = ..." — if there were no phpdocs for $a in advance
  // the reason behind it is equal to op_set to var, described above
  for (auto x : v_list->list()) {
    auto kv = x.as<op_list_keyval>();
    if (auto as_var = kv->var().try_as<op_var>()) {
      const std::string &var_name = as_var->str_val;
      if (!get_found_phpdoc_for_var(var_name) && current_function->get_assumption_for_var(var_name)) {
        Assumption rhs_assum = assume_class_of_expr(current_function, v_list->array(), v_list->array());
        assumption_add_for_var(current_function, var_name, rhs_assum.get_subkey_by_index(kv->key()), v_list);
      }
    }
  }
}

// handle `clone $v`, calc assumptions to be used later
void DeduceImplicitTypesAndCastsPass::on_clone(VertexAdaptor<op_clone> v_clone) {
  v_clone->class_id = assume_class_of_expr(current_function, v_clone->expr(), v_clone).try_as_class();
  kphp_error_return(v_clone->class_id, "`clone` keyword can be used only with instances");
  kphp_error(!v_clone->class_id->is_lambda_class(), "It's forbidden to `clone` lambdas");
  kphp_error(!v_clone->class_id->is_typed_callable_interface(), "It's forbidden to `clone` lambdas");
  kphp_error(!v_clone->class_id->is_builtin() || v_clone->class_id->is_ffi_cdata(), "It's forbidden to `clone` built-in classes");
}

// handle `throw $ex`, calc assumptions to be used later
void DeduceImplicitTypesAndCastsPass::on_throw(VertexAdaptor<op_throw> v_throw) {
  v_throw->class_id = assume_class_of_expr(current_function, v_throw->exception(), v_throw).try_as_class();
}

// handle `function() { ... }` (lambdas were not replaced by lambda classes yet at this step of pipeline)
void DeduceImplicitTypesAndCastsPass::on_lambda(VertexAdaptor<op_lambda> v_lambda) {
  nested_lambdas.emplace_front(v_lambda->func_id);
}

// handle `$obj->field`, calc assumption for $obj and set var_id
void DeduceImplicitTypesAndCastsPass::on_instance_prop(VertexAdaptor<op_instance_prop> v_prop) {
  // lhs->field; we need to infer the class of lhs
  ClassPtr klass = resolve_class_of_arrow_access(current_function, v_prop->instance(), v_prop);
  if (!klass) {   // if so, an error has already been printed
    return;
  }
  v_prop->class_id = klass;

  if (klass->is_ffi_cdata() && v_prop->str_val == "cdata") {
    // a magic low-level property for ffi, it has no backing field; will be replaced by op_ffi_cdata_value_ref later
    return;
  }

  const auto *field = v_prop->class_id->get_instance_field(v_prop->str_val);
  kphp_error_return(field, fmt_format("Field ${} not found in class {}", v_prop->str_val, klass->as_human_readable()));

  v_prop->var_id = field->var;
}
