// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/generics-reification.h"

#include "compiler/data/function-data.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"


/*
 * The word "reification" means deducing generic T to instantiate generic functions/classes.
 *    function f<T>(T $arg) {}
 * Having a call
 *    f(new A)
 * KPHP reifies that T=A and creates an instantiated function f$_$A by fully cloning body of f<T>.
 *
 * Reification fills call->reifiedTs if call->func_id points to a generic function.
 * This is done at DeduceImplicitTypesAndCastsPass.
 *
 * To perform reification, we use assumptions: having `T $arg` and `f(any_expr)`, T=(assumption of any_expr).
 * That's why it perfectly works for instances and partially works for primitives:
 *    f($a)           // T=A
 *    f(1)            // T=int
 *    f(getIntArr())  // T=int[]
 *    f($obj->field)  // T=@var above field
 *    f($int)         // can't reify T unless explicit @var set, because assumptions for vars-primitives aren't stored
 * Assumptions work on var names, as cfg / smart casts / splitting doesn't exist at current execution point,
 * that's why assumptions for vars aren't saved for primitives (the only exception is parameters of lambdas).
 *
 * $arg can depend on T in a more complex way. Say,
 *    function f<T>(T[] $array)
 * KPHP will correctly reify also:
 *    f([new A])      // T=A
 *    f([[true]])     // T=bool[]
 * See `reify_genericT_from_call_argument()`.
 *
 * Generic T may have extends condition and default value in declaration:
 *    function f<T1 : SomeInterface, T2 = mixed, T3 = T2>
 * After reified, call->reifiedTs are checked for extends_hint.
 *
 * Sometimes, generic Ts can't be reified:
 *    f(null)         // T=???
 * To explicitly provide T at a call, there is a special PHP syntax:
 *    f/ *<A>* /(null)
 * If such comment exists, all Ts are taken from there, filling call->reifiedTs = (from comment) + (defaults).
 * If it doesn't exist, auto reification is made.
 * If some Ts aren't reified in the end, it's a compilation error: see `check_reifiedTs_for_generic_func_call()`.
 */

// implemented in DeduceImplicitTypesAndCastsPass cpp
void patch_rhs_casting_to_callable(VertexPtr &rhs, const TypeHintCallable *as_callable, VertexAdaptor<op_func_call> call);

// assumptions for vars aren't saved unless they contain instances (neither for parameters, nor for local vars)
// i.e. even if you write `@var int|false $a`, it won't be saved as an assumption, and `return $a` will assume unknown
// for generics instantiating, like calls `f($a)` we intentionally search for @var even with primitives
static const TypeHint *find_first_phpdoc_for_var(FunctionPtr current_function, const std::string &var_name, VertexPtr stop_at) {
  bool stopped = false;
  VertexAdaptor<op_phpdoc_var> found;

  std::function<void(VertexPtr, VertexPtr)> find_recursive = [&find_recursive, var_name, &stopped, &found](VertexPtr root, VertexPtr stop_at) -> void {
    if (root == stop_at) {
      stopped = true;
    }
    if (stopped || found) {
      return;
    }
    if (auto as_phpdoc_var = root.try_as<op_phpdoc_var>()) {
      if (as_phpdoc_var->var()->str_val == var_name) {
        found = as_phpdoc_var;
      }
    }

    for (auto v : *root) {
      find_recursive(v, stop_at);
    }
  };

  find_recursive(current_function->root->cmd(), stop_at);
  return found ? found->type_hint : nullptr;
}

// `f<T>` could have default type for `T` in declaration, this default should be applied if T wasn't provided/reified
// we also support `f<T1, T2=T1>`, that's why we call phpdoc_replace_genericTs_with_reified() here
// having default types which depend on earlier T is specially useful for generic classes, e.g. `C<T, TCont=Container<T>>`
static inline void apply_defaultTs_if_someTs_not_set(FunctionPtr generic_function, VertexAdaptor<op_func_call> call) {
  for (const auto &itemT : *generic_function->genericTs) {
    if (itemT.def_hint && !call->reifiedTs->find(itemT.nameT)) {
      call->reifiedTs->provideT(itemT.nameT, phpdoc_replace_genericTs_with_reified(itemT.def_hint, call->reifiedTs), call);
    }
  }
}

// when php code contains an explicit generics instantiation `f/*<T1, T2>*/()`, apply it to f
// "T1, T2" was parsed earlier by tokenizer in ParseAndApplyPhpDocForFunction
void apply_instantiationTs_from_php_comment(FunctionPtr generic_function, VertexAdaptor<op_func_call> call, const GenericsInstantiationPhpComment *commentTs) {
  kphp_assert(commentTs != nullptr && !commentTs->vectorTs.empty());
  kphp_error(commentTs->vectorTs.size() <= generic_function->genericTs->size(),
             fmt_format("Mismatch generics instantiation count: waiting {}, got {}", generic_function->genericTs->size(), commentTs->vectorTs.size()));

  for (int i = 0; i < commentTs->vectorTs.size() && i < generic_function->genericTs->size(); ++i) {
    call->reifiedTs->provideT(generic_function->genericTs->itemsT[i].nameT, commentTs->vectorTs[i], call);
  }

  call->reifiedTs->location = call->location;
  apply_defaultTs_if_someTs_not_set(generic_function, call);
}

// when we have a call `f($v)` for a generic function `f<T>`, then T depends on $v
// for $v=1 return int, for $v=$a->field return type of A::$field (even for primitives), etc.
static const TypeHint *reify_instantiationT_from_expr(FunctionPtr current_function, VertexPtr v) {
  const TypeHint *instantiationT = assume_class_of_expr(current_function, v, v).assum_hint;

  // assumptions don't save @var for variables if they are primitives (on purpose, because they can be splitted)
  // that's why, having code `$a = 1; f($a)`, assumption for $a will be null, even if `@var int` written
  // but for the needs for detecting generic T, we allow the user to write @var above, this differs from assumptions
  // (yes, if var will be splitted or smartcasted, and type checking will fail, the PHP programmer can use /*<T>*/ syntax)
  if (!instantiationT && v->type() == op_var) {
    instantiationT = find_first_phpdoc_for_var(current_function, v->get_string(), v);
  }

  return instantiationT;
}

// having a call `f($arg)`, where f is `f<T>`, and $arg is `@param T[] $array`, deduce T
// for example, if an assumption for $arg is A[], then T is A
// this function is called for every generic argument of `f()` invocation
static void reify_genericT_from_call_argument(FunctionPtr current_function, FunctionPtr generic_function, const TypeHint *param_hint, VertexAdaptor<op_func_call> call, VertexPtr &call_arg, bool old_syntax) {
  kphp_assert(param_hint->has_genericT_inside() && generic_function->genericTs != nullptr);

  param_hint = param_hint->unwrap_optional();

  // for @param T, just take an argument passed to this param
  // f(new A) means T=A, f(4) means T=int,
  // f($a->field) means T=(type hint of A::$field), f(someFn()) means T=(return of someFn())
  // f($var) means T=(assumption of $var)
  // f(null) and other (which can't be handled or are not handled in assumptions) don't reify T
  if (const auto *as_genericT = param_hint->try_as<TypeHintGenericT>()) {
    const TypeHint *extends_hint = generic_function->genericTs->get_extends_hint(as_genericT->nameT);

    // here extends_hint is just used for converting 'strlen' to callable
    // this seems ugly, but I couldn't find a better way to reify f<lambda$xxx>, not f<string>
    if (extends_hint && extends_hint->try_as<TypeHintCallable>()) {
      patch_rhs_casting_to_callable(call_arg, extends_hint->try_as<TypeHintCallable>(), call);
      if (stage::has_error()) {
        return;
      }
      // fall through, instantiationT will be set to lambda$xxx
    }

    const TypeHint *instantiationT = reify_instantiationT_from_expr(current_function, call_arg);
    if (old_syntax && !instantiationT) {
      instantiationT = TypeHintPrimitive::create(tp_any);
    }
    if (instantiationT) {
      call->reifiedTs->provideT(as_genericT->nameT, instantiationT, call);
    }
  }

  // for @param T[], if an argument is an array, reify T
  // f($arr_of_A) means T=A if $arr_of_a has an assumption A[]
  // f([1, ...]) means T=int, f($a->field) and f(someFn()) also work, even for arrays of primitives
  if (const auto *as_array = param_hint->try_as<TypeHintArray>()) {
    if (const auto *as_genericT = as_array->inner->try_as<TypeHintGenericT>()) {
      const TypeHint *instantiationT = reify_instantiationT_from_expr(current_function, call_arg);
      if (instantiationT) {
        if (const auto *arg_array = instantiationT->try_as<TypeHintArray>()) {
          call->reifiedTs->provideT(as_genericT->nameT, arg_array->inner, call);
        }
      }
    }
  }

  // for @param callable(...):OutT, if an argument is a lambda, we can not reify OutT here,
  // because we may have not enough information for a lambda to calculate assumptions
  // (say, `map(fn($a) => $a, [new A])`, when analyzing lambda, we don't know $a is A yet)
  // it will be reified (and filled into call) in `patch_rhs_casting_to_callable()`

  // for @param class-string<T>, if an argument is A::class, reify T=A
  if (const auto *as_class_string = param_hint->try_as<TypeHintClassString>()) {
    ClassPtr klassT;
    if (call_arg->type() == op_string) {
      klassT = G->get_class(call_arg->get_string());
      kphp_error_return(klassT, fmt_format("Class {} not found", call_arg->get_string()));
    } else if (call_arg->type() == op_func_call && call_arg->extra_type != op_ex_func_call_arrow) {
      auto args = call_arg.as<op_func_call>()->args();
      if (call_arg->get_string() == "classof" && args.size() == 1) {
        klassT = assume_class_of_expr(current_function, args[0], call).try_as_class();
        kphp_error_return(klassT, fmt_format("Could not detect a class of classof()"));
      }
    }

    kphp_error_return(klassT, "Expected ::class as a parameter, but got not a const string");
    const auto *as_genericT = as_class_string->inner->try_as<TypeHintGenericT>();
    kphp_assert(as_genericT);
    call->reifiedTs->provideT(as_genericT->nameT, TypeHintInstance::create(klassT->name), call);
  }
}

// having a call `f(...)` of a generic function `f<T1, T2>(...)`, deduce T1 and T2
// "auto deducing" for generic arguments is typically called "reification"
// note, that if a PHP user calls f explicitly using `f/*<T1, T2>*/(...)`, this is NOT called:
// this is called for auto-reification, when /*<...>*/ is omitted, meant to be obviously reified
void reify_function_genericTs_on_generic_func_call(FunctionPtr current_function, FunctionPtr generic_function, VertexAdaptor<op_func_call> call, bool old_syntax) {
  kphp_assert(generic_function->is_generic() && generic_function->genericTs);

  call->reifiedTs = new GenericsInstantiationMixin(call->location);

  auto call_args = call->args();
  auto f_called_params = generic_function->get_params();

  // for all params that depend on any generic T, reify T
  // if different arguments depend on the same T, and auto-deducing results in different types,
  // then an error in printed in add_instantiationT()
  for (int i = 0; i < f_called_params.size() && i < call_args.size(); ++i) {
    auto param = f_called_params[i].as<op_func_param>();
    if (!param->type_hint || !param->type_hint->has_genericT_inside()) {
      continue;
    }

    if (param->extra_type == op_ex_param_variadic) {
      reify_genericT_from_call_argument(current_function, generic_function, param->type_hint->try_as<TypeHintArray>()->inner, call, call_args[i], old_syntax);
    } else {
      reify_genericT_from_call_argument(current_function, generic_function, param->type_hint, call, call_args[i], old_syntax);
    }
  }

  apply_defaultTs_if_someTs_not_set(generic_function, call);
}

// `f<T>` can restrict T on declaration, for example `f<T:SomeInterface>`
// on instantiation `f<A>()`, we have to detect whether `A` satisfies this "extends" restriction (implements SomeInterface here)
// this function does this check, filling out_err or leaving it empty
// for `f<T:Some1|Some2>` this function is called for every of OR conditions
static bool does_instantiationT_fit_extends_condition(const TypeHint *instantiationT, const TypeHint *extends_cond, std::string &out_err) {
  if (const auto *as_instance = extends_cond->try_as<TypeHintInstance>()) {
    const auto *inst_instance = instantiationT->try_as<TypeHintInstance>();
    if (!inst_instance) {
      out_err = fmt_format("It should be a class extending {}", as_instance->full_class_name);
    } else if (!as_instance->resolve()->is_parent_of(inst_instance->resolve())) {
      out_err = fmt_format("It does not {} {}", as_instance->resolve()->is_interface() ? "implement" : "extend", as_instance->full_class_name);
    } else {
      out_err = "";
    }

  } else if (extends_cond->try_as<TypeHintCallable>()) {
    if (!instantiationT->try_as<TypeHintCallable>()) {
      out_err = "Seems that something is wrong with a callable passed, it's not callable actually";
    } else {
      out_err = "";
    }

  } else if (extends_cond->try_as<TypeHintObject>()) {
    if (!instantiationT->try_as<TypeHintInstance>()) {
      out_err = fmt_format("It should be any instance");
    } else {
      out_err = "";
    }

  } else if (extends_cond->try_as<TypeHintOptional>() || extends_cond->try_as<TypeHintPipe>()) {
    kphp_assert(0);

  } else {
    // this can occur for `f<T: int|string>` when called for each of conditions; do that simple for now
    if (instantiationT != extends_cond) {
      out_err = fmt_format("Expected {}", extends_cond->as_human_readable());
    } else {
      out_err = "";
    }
  }

  return out_err.empty();
}

// when `f<T>` is declared as `f<T:SomeInterface>`, and we have an instantiation `f<A>()`,
// check that A corresponds SomeInterface
// for `f<T: int|string>`, check all conditions
// we check only hints that are supported in declarations, see check_declarationT_extends_hint()
static void check_reifiedT_extends_hint(const std::string &nameT, const TypeHint *instantiationT, const TypeHint *extends_hint) {
  std::string err;

  if (extends_hint->try_as<TypeHintInstance>() || extends_hint->try_as<TypeHintCallable>() || extends_hint->try_as<TypeHintObject>()) {
    // 'T: SomeInterface' / 'T: BaseClass' / 'T: callable' / 'T: object'
    does_instantiationT_fit_extends_condition(instantiationT->unwrap_optional(), extends_hint, err);

  } else if (const auto *as_pipe = extends_hint->try_as<TypeHintPipe>()) {
    // 'T: int|string' / 'T: SomeInterface|SomeInterface2'
    for (const TypeHint *item : as_pipe->items) {
      if (does_instantiationT_fit_extends_condition(instantiationT->unwrap_optional(), item, err)) {
        return;
      }
    }
    err = "It does not fit any of specified conditions";

  } else {
    kphp_assert(0 && "unexpected extends_hint branch");
  }

  kphp_error(err.empty(),
             fmt_format("Calculated generic <{}> = {} breaks condition '{}:{}'.\n{}", nameT, TermStringFormat::paint_green(instantiationT->as_human_readable()), nameT, extends_hint->as_human_readable(), err));
}

// when all generic T of `f<T1, T2, ...>()` have been auto-reified or parsed from /*<php comment>*/,
// check their correctness
void check_reifiedTs_for_generic_func_call(const GenericsDeclarationMixin *genericTs, VertexAdaptor<op_func_call> call, bool old_syntax) {
  kphp_assert(call->reifiedTs);

  for (const auto &itemT : *genericTs) {
    const TypeHint *instantiationT = call->reifiedTs->find(itemT.nameT);
    if (!instantiationT || instantiationT->has_genericT_inside() || instantiationT->has_autogeneric_inside()) {
      kphp_error(0, fmt_format("Couldn't reify generic <{}> for call.\n{}", itemT.nameT, genericTs->prompt_provide_commentTs_human_readable(call)));
      continue;
    }
    // prohibit f<array>() and others any-containing (even if set explicitly)
    if (!old_syntax) {
      kphp_error(!instantiationT->has_tp_any_inside(), fmt_format("Generics <{}> = {} should be strict, it must not contain 'any' inside", itemT.nameT, TermStringFormat::paint_green(instantiationT->as_human_readable())));
    }

    if (itemT.extends_hint) {
      check_reifiedT_extends_hint(itemT.nameT, instantiationT, itemT.extends_hint);
    }
  }
}
