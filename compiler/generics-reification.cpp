// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/generics-reification.h"

#include "compiler/data/function-data.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/function-pass.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"


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
 * Generic T may be variadic:
 *    function f<...TArg>(TArg ...$args)
 * On a call with 2 arguments, a function `f$n2<TArg1, TArg2>(TArg1 $args$1, TArg2 $args$2)` is implicitly created.
 * Consider `convert_variadic_generic_function_accepting_N_args()`.
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
    auto real_call_arg = VertexUtil::get_actual_value(call_arg);
    if (real_call_arg->type() == op_string) {
      klassT = G->get_class(real_call_arg->get_string());
      kphp_error_return(klassT, fmt_format("Class {} not found", real_call_arg->get_string()));
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

// implementation of variadic generic function specialized by N:
// when `f<...TArg>(...$args)` is called with N=2 arguments, a function `f$n2` is created,
// replacing "...$args" with "$args$1, $args$2" along with ...TArgs
// $args can be used only in special contexts:
// * forwarding `anotherF(0, ...$args)` (replaced by `anotherF(0, $args$1, $args$2)`, same below)
// * merging into an array `[0, ...$args]`, particularly you can append an argument `anotherF(...[...$args, 0])`
// $args can not be used in foreach, accessed by an index, etc.: it's a compilation error
class ReplaceVariadicArgWithNArgsPass : public FunctionPassBase {
  std::string variadic_var_name;    // $args
  FunctionPtr generic_function;     // f (but current_function is f$n2)
  int n_variadic;                   // N=2
  VertexPtr replaced_v_for_on_exit; // see on_exit_vertex()

public:
  explicit ReplaceVariadicArgWithNArgsPass(FunctionPtr generic_function, int n_variadic)
    : generic_function(std::move(generic_function))
    , n_variadic(n_variadic) {}

  std::string get_description() override {
    return "Replace $args with $args$1, $args$2";
  }

  void on_start() override {
    auto params = generic_function->get_params();
    auto last_param = params[params.size() - 1].as<op_func_param>();
    kphp_assert(last_param->extra_type == op_ex_param_variadic);
    variadic_var_name = last_param->var()->str_val;
  }

  void on_finish() override {
    current_function->has_variadic_param = false;
  }

  VertexPtr on_exit_vertex(VertexPtr root) override {
    if (replaced_v_for_on_exit) {
      root = replaced_v_for_on_exit;
      replaced_v_for_on_exit = VertexPtr{};
      run_function_pass(root, this);
      return root;
    }

    if (is_op_var_generic(root)) {
      kphp_error(0, fmt_format("Variadic argument ...${} used incorrectly in a variadic generic.\nIt can not be replaced with N={} copies here.\nFor example, f(${}) is incorrect, but f(...${}) is okay.", variadic_var_name, n_variadic, variadic_var_name, variadic_var_name));
    }

    return root;
  }

  // check: root == $args
  inline bool is_op_var_generic(VertexPtr root) {
    return root->type() == op_var && root.as<op_var>()->str_val == variadic_var_name;
  }

  // check: root == ...$args
  inline bool is_op_varg_op_var_generic(VertexPtr root) {
    return root->type() == op_varg && is_op_var_generic(root.as<op_varg>()->array());
  }

  // creates $args$i
  inline VertexAdaptor<op_var> create_ith_op_var(VertexPtr v_get_location, int i) {
    auto ith_var = VertexAdaptor<op_var>::create().set_location(v_get_location->location);
    ith_var->str_val = variadic_var_name + "$" + std::to_string(i);
    return ith_var;
  }

  bool user_recursion(VertexPtr root) override {
    if (auto as_param_list = root.try_as<op_func_param_list>()) {
      replaced_v_for_on_exit = replace_varg_in_func_param_list(as_param_list);
    } else if (auto as_call = root.try_as<op_func_call>()) {
      replaced_v_for_on_exit = replace_varg_in_func_call(as_call);
      // count($args) is ok, replace it with N at compile-time
      if (as_call->str_val == "count" && as_call->size() == 1 && as_call->extra_type != op_ex_func_call_arrow && is_op_var_generic(as_call->front())) {
        replaced_v_for_on_exit = VertexUtil::create_int_const(n_variadic);
      }
    }

    return !!replaced_v_for_on_exit;
  }

  // function f$n2(...$args) => function f$n2($args$1, $args$2)
  VertexPtr replace_varg_in_func_param_list(VertexAdaptor<op_func_param_list> param_list) {
    std::vector<VertexAdaptor<op_func_param>> new_params_vector;
    bool occured_varg = false;

    for (VertexPtr param : param_list->params()) {
      auto p = param.as<op_func_param>();
      if (p->extra_type == op_ex_param_variadic && p->type_hint) {
        occured_varg = true;
        std::string nameT = p->type_hint->try_as<TypeHintArray>()->inner->try_as<TypeHintGenericT>()->nameT;
        for (int i = 1; i <= n_variadic; ++i) {
          auto ith_param = VertexAdaptor<op_func_param>::create(create_ith_op_var(p, i)).set_location(p);
          ith_param->type_hint = TypeHintGenericT::create(nameT + std::to_string(i));
          new_params_vector.emplace_back(ith_param);
        }
      } else {
        new_params_vector.emplace_back(p);
      }
    }
    if (!occured_varg) {
      return {};
    }

    auto new_param_list = VertexAdaptor<op_func_param_list>::create(new_params_vector).set_location(current_function->root);
    return new_param_list;
  }

  // anotherF(...$args) => anotherF($args$1, $args$2)
  // [...$args] => [$args$0, $args$1] also here,
  //    because it was already replaced by a func call array_merge_spread($args), so => array_merge_spread([$args$0], [$args$1])
  VertexPtr replace_varg_in_func_call(VertexAdaptor<op_func_call> call) {
    std::vector<VertexPtr> new_args_vector;
    const bool is_array_creation = call->str_val == "array_merge_spread" && call->extra_type != op_ex_func_call_arrow;
    bool occured_varg = false;
    new_args_vector.reserve(call->size() - 1 + n_variadic);

    for (VertexPtr arg : call->args()) {
      if (is_op_varg_op_var_generic(arg)) {   // f(1, 2, ...$here)
        occured_varg = true;
        for (int i = 1; i <= n_variadic; ++i) {
          new_args_vector.emplace_back(create_ith_op_var(arg, i));
        }
      } else if (is_array_creation && is_op_var_generic(arg)) {   // [1, 2, ...$here] (actually array_merge_spread(1, 2, $here))
        occured_varg = true;
        for (int i = 1; i <= n_variadic; ++i) {
          std::vector<VertexPtr> ith_wrap{create_ith_op_var(arg, i)};
          new_args_vector.emplace_back(VertexAdaptor<op_array>::create(ith_wrap).set_location(arg));
        }
      } else {
        new_args_vector.emplace_back(arg);
      }
    }
    if (!occured_varg) {
      return {};
    }

    auto new_call = VertexAdaptor<op_func_call>::create(new_args_vector).set_location(call);
    new_call->str_val = call->str_val;
    new_call->func_id = call->func_id;
    new_call->extra_type = call->extra_type;
    new_call->auto_inserted = call->auto_inserted;
    return new_call;
  }
};

// when a variadic generic `f<...TArg>(...$args)` is called with 2 arguments,
// a function `f$n2` is created, replacing "...TArg" with "TArg1, TArg2" in declaration
// and "...$args" with "$args$1, $args$2" in body (see a pass above)
FunctionPtr convert_variadic_generic_function_accepting_N_args(FunctionPtr generic_function, int n_variadic) {
  std::string name = generic_function->name + "$n" + std::to_string(n_variadic);
  FunctionPtr f_converted;

  G->operate_on_function_locking(name, [generic_function, n_variadic, name, &f_converted](FunctionPtr &f_ht) {
    if (f_ht) {
      f_converted = f_ht;
      return;
    }

    f_converted = FunctionData::clone_from(generic_function, name);
    f_converted->genericTs = GenericsDeclarationMixin::create_for_function_cloning_from_variadic(generic_function, n_variadic);
    kphp_error(!f_converted->genericTs->empty(), "Variadic generic used with zero arguments, and there are no more generic Ts besides");

    std::string stage_backup_name = stage::get_stage_info_ptr()->name;
    Location stage_backup_location = stage::get_stage_info_ptr()->location;
    ReplaceVariadicArgWithNArgsPass pass(generic_function, n_variadic);
    run_function_pass(f_converted, &pass);
    stage::get_stage_info_ptr()->name = stage_backup_name;
    stage::get_stage_info_ptr()->location = stage_backup_location;

    f_ht = f_converted;
  });

  return f_converted;
}
