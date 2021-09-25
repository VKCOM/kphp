// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/deduce-implicit-types-and-casts.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/lambda-utils.h"
#include "compiler/name-gen.h"
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
 * Having a call in terms of template functions,
 *    f($obj, function($o) { ... });
 * We need an assumption for $obj, to instantiate f(), to auto-infer $o as an argument.
 *
 * As a result, this step simultaneously calculates all assumptions on demand.
 * Not only for ->arrowCalls(), but for ->fields and other cases – everything except binding ()-invocations, that's resolved later.
 *
 * Here we also bind func calls (set func_id of every op_func_call). It seems logical to do this while resolving assumptions.
 * func_id becomes valid for functions and methods, but for template functions, it still points to a template function.
 * (template functions are instantiated a bit later, and func_id is reassigned to an instantiation)
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
 * Concluding, this step does the following, simultaneously:
 * - resolves func calls and binds func_id to every op_func_call
 * - modifies arguments of functions or rhs at assignments to fit phpdoc (lhs)
 * - when an ->arrowCall() found, calculates assumptions to bind func_id
 * - when an ->field access found, calculates assumptions to bind var_id
 * - calculates assumptions for some other needs for later passes
 * - deduces types of lambdas arguments passed to array_map() or similar, as well as to typed callables
 * - deduces <T> for template function calls, saving it to call->instantiation_list
 * - some other minors, read the code
 */


// having an array_map type_hint `callable(^2[*] $x)` and a call `array_map(fn($a) => ..., [new A]),
// deduce that $a is A, based on ^arg refs and assumptions
static const TypeHint *patch_type_hint_with_argref_in_context_of_call(const TypeHint *type_hint, FunctionPtr current_function, VertexAdaptor<op_func_call> call) {
  const TypeHint *replaced = type_hint->replace_children_custom([current_function, call](const TypeHint *child) {
    if (const auto *as_arg_ref = child->try_as<TypeHintArgRef>()) {
      if (auto call_arg = GenTree::get_call_arg_ref(as_arg_ref->arg_num, call)) {
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

// having an rhs (fn(), or 'strlen', or [$obj, 'method']) passed as typed/untyped callable,
// do nesessary modifications of rhs
// the argument `call` is valid for func calls (to resolve ^2), but is empty for assignments
// mind that lambdas and non-lambdas passed to built-in functions are represented as op_callback_of_builtin
static void patch_rhs_casting_to_callable(VertexPtr &rhs, const TypeHintCallable *as_callable, VertexAdaptor<op_func_call> call) {
  bool is_extern_func_param = call && call->func_id && call->func_id->is_extern();
  InterfacePtr typed_interface = as_callable->is_typed_callable() && as_callable->is_typedata_constexpr() ? as_callable->get_interface() : InterfacePtr{};

  if (as_callable->is_typed_callable() && rhs->type() == op_lambda) {
    // function(...) { ... } passed as callable(int):string (to array_map in particular)
    FunctionPtr f_lambda = rhs.as<op_lambda>()->func_id;
    auto f_lambda_params = f_lambda->get_params();
    for (int i = 0; i < f_lambda_params.size() && i < as_callable->arg_types.size(); ++i) {
      auto l_param = f_lambda_params[i].as<op_func_param>();
      if (!l_param->type_hint) {
        const TypeHint *arg_type_hint = as_callable->arg_types[i];
        l_param->type_hint = arg_type_hint->has_argref_inside()
                             ? patch_type_hint_with_argref_in_context_of_call(arg_type_hint, stage::get_function(), call)
                             : arg_type_hint;
      }
    }
    if (!f_lambda->return_typehint) {
      f_lambda->return_typehint = as_callable->return_type;
    }
    rhs.as<op_lambda>()->lambda_class = typed_interface;  // save for the next pass

  } else if (rhs->type() == op_string) {
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

  } else if (rhs->type() == op_array && rhs->size() == 2 && rhs->front()->type() == op_string && rhs->back()->type() == op_string) {
    // ['A', 'staticMethod']
    std::string func_name = rhs->front()->get_string() + "$$" + rhs->back()->get_string();
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

  } else if (rhs->type() == op_array && rhs->size() == 2 && rhs->back()->type() == op_string) {
    // [$obj, 'method'] or [getObject(), 'method'] or similar
    // pay attention, that $obj (rhs->front()) is captured in an auto-created wrapping lambda or in a c++ lambda in case of extern
    if (is_extern_func_param) {
      auto v_callback = VertexAdaptor<op_callback_of_builtin>::create(rhs->front()).set_location(rhs);
      v_callback->str_val = rhs->back()->get_string();
      rhs = v_callback;
    } else {
      auto v_lambda = VertexAdaptor<op_lambda>::create().set_location(rhs);
      v_lambda->func_id = generate_lambda_from_string_or_array(stage::get_function(), rhs->back()->get_string(), rhs->front(), rhs);
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
  // to avoid reduntant work, every type hint has a special flag whether it potentially can requied an rhs cast
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

  if (const auto *as_callable = lhs_type_hint->try_as<TypeHintCallable>()) {
    // @param callable — extracted to a separate method, not to mess here
    patch_rhs_casting_to_callable(rhs, as_callable, call);
  }
  if (const auto *as_array = lhs_type_hint->try_as<TypeHintArray>()) {
    // @param callable[], cast every rhs array's element
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
  if (const auto *as_tuple = lhs_type_hint->try_as<TypeHintTuple>()) {
    // @param tuple(int, callable(...):void), cast correspondent arguments of a passed tuple
    if (auto rhs_as_tuple = rhs.try_as<op_tuple>()) {
      for (int i = 0; i < as_tuple->items.size() && i < rhs_as_tuple->size(); ++i) {
        patch_rhs_casting_to_lhs_type(rhs_as_tuple->args()[i], as_tuple->items[i], call);
      }
    }
  }
  if (const auto *as_shape = lhs_type_hint->try_as<TypeHintShape>()) {
    // @param shape(cb: callable(...):int), cast correspoinding arguments of a passed shape
    if (auto rhs_as_shape = rhs.try_as<op_shape>()) {
      for (auto sub_expr : rhs_as_shape->args()) {
        const std::string &shape_key = GenTree::get_actual_value(sub_expr->front())->get_string();
        if (const TypeHint *lhs_type_at_key = as_shape->find_at(shape_key)) {
          patch_rhs_casting_to_lhs_type(sub_expr->back(), lhs_type_at_key, call);
        }
      }
    }
  }
}

// having a call `f(...)` of a template function `f<T>(...)`, deduce `T`
// for now, we don't have rich generics functions implementations, only template functions are supported
// (it means, that we can't express `f<T>(T[] $array)`, only `f<T>(T $arg)`, so T = assumption for $arg)
// in the future, we'll support vectors/maps and enhanced generics, and this will be enriched
static void patch_instantiation_list_on_generics_func_call(FunctionPtr template_function, VertexAdaptor<op_func_call> call, FunctionPtr current_function) {
  kphp_assert(template_function->is_template() && template_function->generics_declaration);

  auto *instantiation_list = new GenericsInstantiationMixin;
  VertexRange func_args = template_function->get_params();

  for (int i = 0; i < func_args.size(); ++i) {
    auto param = func_args[i].as<op_func_param>();
    if (!param->type_hint || !param->type_hint->has_genericsT_inside()) {
      continue;
    }
    kphp_assert(param->type_hint->try_as<TypeHintGenericsT>());     // only `T $arg`
    const std::string &nameT = param->type_hint->try_as<TypeHintGenericsT>()->nameT;

    VertexPtr call_arg = GenTree::get_call_arg_for_param(call, param, i);
    kphp_error_return(call_arg, fmt_format("missed {}th argument in calling a template function {}", i, call->func_id->as_human_readable()));

    Assumption assumption = assume_class_of_expr(current_function, call_arg, call);
    const TypeHint *instantiation_hint = assumption.assum_hint ?: TypeHintPrimitive::create(tp_any);
    // we'd better not auto-instantiate tp_any if can't detect an instance,
    // but for current template function syntax there is no explicit way to specify an instantiation
    // so, leave this for now, and when we implement generics, we'll reconsider this

    instantiation_list->add_instantiationT(nameT, instantiation_hint);
  }
  kphp_assert(template_function->generics_declaration->size() == instantiation_list->size());

  call->instantiation_list = instantiation_list;
  call->instantiation_list->location = call->location;
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
        return GenTree::conv_to_lval<tp_array>(rhs);
      case tp_int:
        return GenTree::conv_to_lval<tp_int>(rhs);
      case tp_string:
        return GenTree::conv_to_lval<tp_string>(rhs);
      case tp_mixed:
        return rhs;
      default:
        kphp_error (0, "Too hard rule for cast a param with ref_flag");
        return rhs;
    }
  }
  switch (tp) {
    case tp_int:
      return GenTree::conv_to<tp_int>(rhs);
    case tp_bool:
      return GenTree::conv_to<tp_bool>(rhs);
    case tp_string:
      return GenTree::conv_to<tp_string>(rhs);
    case tp_float:
      return GenTree::conv_to<tp_float>(rhs);
    case tp_array:
      return GenTree::conv_to<tp_array>(rhs);
    case tp_regexp:
      return GenTree::conv_to<tp_regexp>(rhs);
    case tp_mixed:
      return GenTree::conv_to<tp_mixed>(rhs);
    default:
      return rhs;
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
      // we had an assumption for $v already ($v was used to bind arrow calls or instantiate a template), check that phpdoc equals
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
// if f() is a template function `f<T1,T2,...>(...)`, deduce instantiation T's (save call->instantiation_list)
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

  if (f_called->is_template() && !call->instantiation_list) {
    patch_instantiation_list_on_generics_func_call(f_called, call, current_function);
  }
}

// a helper to print a human-readable error for `f()` when f not found
// the return value is not used, it's just to make the code shorter by using `return kphp_error(...)`
int DeduceImplicitTypesAndCastsPass::print_error_unexisting_function(std::string unexisting_func_name) {
  unexisting_func_name = vk::replace_all(unexisting_func_name, "$$", "::");
  unexisting_func_name = vk::replace_all(unexisting_func_name, "$", "\\");

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
  // for cast params (::: in functions.txt or '@kphp-infer cast') we add conversions automatically (implicit casts),
  // unless the file from where we're calling this function is annotated with strict_types=1
  if (param->is_cast_param && is_implicit_cast_allowed(current_function->file_id->is_strict_types, param->type_hint)) {
    call_arg = implicit_cast_call_arg_to_cast_param(call_arg, param->type_hint, param->var()->ref_flag);
    return;
  }

  // for `f('strlen')`, convert strlen to a lambda, so that `f<T>` would be instantiated correctly (not as f<string>)
  // note, that patching arguments (here) is done _before_ calculating call->instantiation_list
  // that's why we need to reify generics arguments at first (here), <T> is calculated based on this reification
  // for now, we accept only "callable"; later, when generics are supported, this will be enriched and extracted to a separate function
  if (param->type_hint->has_genericsT_inside()) {
    FunctionPtr f_called = call->func_id;
    kphp_assert(f_called->is_template() && f_called->generics_declaration);
    if (const auto *as_genericsT = param->type_hint->try_as<TypeHintGenericsT>()) {
      const TypeHint *extends_hint = call->instantiation_list ? call->instantiation_list->find(as_genericsT->nameT)
                                                              : f_called->generics_declaration->find(as_genericsT->nameT);
      if (extends_hint && extends_hint->try_as<TypeHintCallable>()) {
        patch_rhs_casting_to_lhs_type(call_arg, TypeHintCallable::create_untyped_callable(), call);
      }
    }
  }

  // maybe, we need to hack the call_arg expression based on types
  // for example: an op_lambda passed to a typed callable @param is auto-inherited from a typed interface
  // for example: a string passed to a callable is converted to op_callback_of_builtin
  if (param->extra_type != op_ex_param_variadic) {
    patch_rhs_casting_to_lhs_type(call_arg, param->type_hint, call);
  } else if (param->type_hint->try_as<TypeHintArray>()) {
    patch_rhs_casting_to_lhs_type(call_arg, param->type_hint->try_as<TypeHintArray>()->inner, call);
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
  kphp_error(!v_clone->class_id->is_builtin(), "It's forbidden to `clone` built-in classes");
}

// handle `throw $ex`, calc assumptions to be used later
void DeduceImplicitTypesAndCastsPass::on_throw(VertexAdaptor<op_throw> v_throw) {
  v_throw->class_id = assume_class_of_expr(current_function, v_throw->exception(), v_throw).try_as_class();
}

// handle `$obj->field`, calc assumption for $obj and set var_id
void DeduceImplicitTypesAndCastsPass::on_instance_prop(VertexAdaptor<op_instance_prop> v_prop) {
  // lhs->field; we need to infer the class of lhs
  ClassPtr klass = resolve_class_of_arrow_access(current_function, v_prop->instance(), v_prop);
  if (!klass) {   // if so, an error has already been printed
    return;
  }
  v_prop->class_id = klass;

  const auto *field = v_prop->class_id->get_instance_field(v_prop->str_val);
  kphp_error_return(field, fmt_format("Field ${} not found in class {}", v_prop->str_val, klass->as_human_readable()));

  v_prop->var_id = field->var;
}
