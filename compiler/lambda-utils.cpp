// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/lambda-utils.h"

#include "compiler/compiler-core.h"
#include "compiler/gentree.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "compiler/vertex-util.h"

/*
 * How do lambdas work.
 *
 * When we have
 *    $a = function() { echo 1; };
 *    $a();
 * AT FIRST (in gentree) it's converted to
 *    $a = op_lambda;       // op_set { op_var $a, op_lambda (refs to FunctionPtr lambda$xxx type = func_lambda) }
 *    $a();                 // op_invoke_call { op_var $a }
 * lambda$xxx, which body is `echo 1;`, also travels through pipeline like a regular function.
 * THEN (while instantiating generics and lambdas) op_lambda is converted to a lambda class
 *    $a = Lambda$XXX$$__construct(op_alloc);
 *    $a();
 * lambda$xxx (which is `echo 1;`) is left as is, and __invoke method just recalls it:
 *    class Lambda$XXX {
 *        // it has no fields here; it has fields if a lambda captures variables
 *        function __construct($this);                  // does nothing, as there are no fields
 *        function __invoke() { return lambda$xxx(); }  // calls 'echo 1;', returns void
 *    }
 * __construct() and __invoke() methods are not lambdas (e.g. `f->is_lambda()` returns false for them)
 * But the class itself is a lambda-class (`c->is_lambda_class()` is true)
 * THEN (while binding invoke calls in ConvertInvokeToFuncCallPass) `$a()` is replaced with `$a->__invoke()`:
 *    $a = Lambda$XXX$$__construct(op_alloc);
 *    $a->__invoke();       // which is actually Lambda$XXX$__invoke($a): op_func_call { op_var $a }
 *
 * Replacing op_invoke_call `$a()` with op_func_call `$a->__invoke()` is done in ConvertInvokeToFuncCallPass.
 * This step is after a sync pipe, after all lambdas and generics are expanded, and can't be done earlier.
 * Why? Consider this hard example:
 *    function hardDemo<T>(T $arg) { return fn() => fn() => $arg; }
 *    hardDemo($obj)()()->objMethod();
 * Assumptions for this could be calculated (to bind ->objMethod()) while deducing types,
 * but in order to replace `()` with `__invoke()`, we really have to wait until everything is instantiated,
 * because at the moment when `()()` is called, lambdas inside `hardDemo<T>` could not yet be replaced by classes actually.
 * All in all, see ConvertInvokeToFuncCallPass which binds op_invoke_call after a sync pipe of all lambdas/generics.
 *
 * Lambdas and uses.
 * A lambda can capture external variables via use(), or implicitly if an arrow, or parent $this.
 *    $a = function() use ($x) { return $this->field + $x; };
 * In gentree, it's left as is:
 *    $a = op_lambda;       // refs to lambda$xxx with type = func_lambda, uses_list = [$this, $x]
 * After converting, it becomes
 *    $a = Lambda$XXX$$__construct(op_alloc, $this, $x);
 * And the class looks like this:
 *    class Lambda$XXX {
 *        A $this;
 *        int $x;
 *        function __construct($this, A captured$this, int $x);   // assigns the fields
 *        function __invoke() { return lambda$xxx(this->$this, this->$x); }
 *    }
 * At the same time, func_lambda starts accepting two more arguments (extra_type = op_ex_param_from_use):
 *    function lambda$xxx($this, $x)    // body is unchanged: return $this->field + $x;
 * So, params from uses are prepended to all lambdas (in InstantiateGenericsAndLambdasPass):
 *    function ($a, $b) use ($c, $d) { ... }
 * Will be transformed into
 *    function ($c, $d, $a, $b) { ... body unchanged }      // $c and $d have extra_type = op_ex_param_from_use
 *
 * Untyped callables.
 * The keyword `callable` is very weird, but we have to support it, as its usage is quite common.
 *    function callMe(callable $fn)
 * `callable` param makes this function be a generic one implicitly, e.g.
 *    function callMe<CallbackT : callable>(CallbackT $fn)
 * When `callMe()` is called with different lambdas, they are actually instantiated by Lambda$XXX instances,
 * so there are many clones: callMe$_$Lambda$XXX1, callMe$_$Lambda$XXX2, etc.
 * When `$fn()` inside the body is invoked,
 *    $fn()
 * it's transformed to an __invoke() method call, since $fn is an instance of a lambda class:
 *    $fn->__invoke()     // which really is Lambda$XXX1$$__invoke($fn)
 * This transformation is done in ConvertInvokeToFuncCallPass, described above.
 * Remember, that we can pass 'functionName' or ['className', 'method'] as callables. This is described below.
 *
 * Typed callables.
 * 'callable(T1,T2,...):ReturnT' is a regular type, it can occur anywhere in phpdoc.
 *    function callMe(callable(int):void $fn)
 * When a function accepts a typed callable as @param, it implicitly accepts an interface:
 *    function callMe(ITyped$callable_int__void $fn)
 * Callables (lambda classes) that are passed to these params, do implicitly implement this interface:
 *    callMe(function($a) { })
 * Is as previously converted to
 *    callMe(Lambda$XXX$$__construct(op_alloc))
 * Where this class auto-implements that interface:
 *    class Lambda$XXX implements ITyped$callable_int__void
 * Notably, all lambdas that are passed/assigned to 'callable(int):void' implement one and the same interface,
 * so __invoke() of that interface switches through all cases.
 * As a consequence, if one of that lambdas throws, an interface __invoke() throws also, same for resumable.
 *
 * Callbacks passed as strings/arrays.
 * If a PHP function accepts a callable, this is a valid PHP syntax:
 *    f('strlen');
 *    f([A::class, 'staticMethod']);
 *    f([$a, 'instanceMethod']);
 * This is done via generating a wrapping lambda class, passing it as argument. This of it as:
 *    f(fn($s) => strlen($s));
 *    f(fn() => A::staticMethod());
 *    f(fn(...$args) => $a->instanceMethod(...$args));
 * If we pass a string/array to a typed callable, this implicit lambda class also implements a typed interface.
 * Note, that f() is considered to be a PHP function, not a built-in,
 * because passing lambdas and non-lambdas to built-in callbacks involve another AST, see below.
 *
 * Lambdas and PHPDocs, deducing implicit types and casts.
 * 'strlen' is converted to a callable in DeduceImplicitTypesAndCastsPass.
 * At the same time,
 *    / ** @var callable * /
 *    $strlen = 'strlen';
 *    $strlen('asdf');
 * also works, at the very same moment: we have an expected lhs type (callable) and rhs value, which is auto-casted,
 * regardless of whether lhs is an argument of a variable controlled by phpdoc before setting to it.
 * At the same time, assumptions are calculated on demand, because
 *    $obj->foo('strlen')
 * has to know the assumption for $obj.
 * At the same time, func_id for every op_func_call is bound.
 * At the same time, generic Ts are auto-reified.
 * All these computations are done simultaneously in DeduceImplicitTypesAndCastsPass, see comments there.
 *
 * Lambdas passed to built-in functions.
 * There is a special optimization to avoid creating extra lambda classes, for situations like
 *    array_map(fn($x) => $x + 1, [1,2,3]);
 * It's represented as follows:
 *    array_map(op_callback_of_builtin func_id = lambda$xxx { no uses => no children }, [1,2,3]);
 * Which is codegenerated to C++ code like
 *    f$array_map([](auto &&... args) { return f$lambda$xxx(std::forward(args)); }, [1,2,3]);
 * When a lambda has uses_list, it's converted to C++ captured variables. Say,
 *    array_map(fn($x) => $x + $add, [1,2,3]);
 * It's represented as follows:
 *    array_map(op_callback_of_builtin func_id = lambda$xxx { op_var $add }, [1,2,3]);
 * Remember, that lambda$xxx now has an argument $add prepended:
 *    function lambda$xxx($add, $x) { return $x + $add; }
 * And in C++:
 *    f$array_map([captured1 = $add](auto &&... args) { return f$lambda$xxx(captured1, std::forward(args)); }, [1,2,3]);
 * Not only variables can be captured. For instance, if a captured $add is smartcasted, it could look like
 *    f$arrap_map([captured1 = check_not_false($add).val()](auto &&... args) ...
 *
 * Non-lambdas passed to built-in functions.
 * op_callback_of_builtin ALWAYS wraps a callback passed to any built-in function, even if it's no a lambda.
 *    array_map('strval', [1,2,3]);
 * Will be represented as
 *    array_map(op_callback_of_builtin func_id = strval { no children }, [1,2,3]);
 * And in C++:
 *    f$array_map([](auto &&... args) { return f$strlen(std::forward(args)); }, [1,2,3]);
 * Remember, that this also is a valid PHP syntax:
 *    register_shutdown_function([$obj, 'onShut']);
 * Will be represented as
 *    register_shutdown_function(op_callback_of_builtin func_id = SomeClass::onShut { op_var $obj }, [1,2,3]);
 * If just a variable or something unknown is passed to a built-in,
 *    array_map($f, [1,2,3]);
 *    array_map(getProcessor(), [1,2,3]);
 * It means that we are accepting a lambda instance — and bind func_id to an __invoke method in that object.
 * Converting a passed argument to op_callback_of_builtin is done also in DeduceImplicitTypesAndCastsPass,
 * but binding func_id is done in ConvertInvokeToFuncCallPass (unless it's a lambda function, described above),
 * for the same reason that we can bind op_invoke_call only at that moment.
 */


// this function adds variables from uses_list to arguments
//    function($x) use ($a)
// before: lambda$xxx($x)
// after:  lambda$xxx($a, $x), $a extra_type = op_ex_param_from_use
void patch_lambda_function_add_uses_as_arguments(FunctionPtr f_lambda) {
  kphp_assert(f_lambda && f_lambda->is_lambda());
  if (f_lambda->uses_list.empty()) {
    return;
  }

  std::vector<VertexAdaptor<op_func_param>> func_params;

  for (auto var_as_use : f_lambda->uses_list) {
    auto var_as_param = VertexAdaptor<op_var>::create().set_location(f_lambda->root);
    var_as_param->str_val = var_as_use->str_val;
    auto param_from_use = VertexAdaptor<op_func_param>::create(var_as_param).set_location(f_lambda->root);
    param_from_use->extra_type = op_ex_param_from_use;
    func_params.emplace_back(param_from_use);
  }
  for (VertexPtr p : f_lambda->get_params()) {
    func_params.emplace_back(p.as<op_func_param>());
  }

  f_lambda->root->param_list_ref() = VertexAdaptor<op_func_param_list>::create(func_params);
}

// this function creates the __invoke() method in a lambda class, that calls a lambda function
//    function($x) use ($a)
//  was turned into
//    lambda$xxx($a, $x)
// the __invoke() method of a lambda class will look like
//    __invoke($x) { return lambda$xxx($this->a, $x); }
static FunctionPtr generate_invoke_method_for_lambda_class(FunctionPtr f_lambda, ClassPtr c_lambda) {
  std::vector<VertexPtr> call_args;
  std::vector<VertexAdaptor<op_func_param>> invoke_params;
  invoke_params.emplace_back(ClassData::gen_param_this({}));

  for (VertexPtr p : f_lambda->get_params()) {
    auto v_var = p.as<op_func_param>()->var();

    if (p->extra_type == op_ex_param_from_use) {
      auto arg_this_prop = VertexAdaptor<op_instance_prop>::create(ClassData::gen_vertex_this({}));
      arg_this_prop->str_val = v_var->str_val;
      call_args.emplace_back(arg_this_prop);
    } else if (p->extra_type == op_ex_param_variadic) {
      call_args.emplace_back(VertexAdaptor<op_varg>::create(v_var.clone()));
      invoke_params.emplace_back(p.as<op_func_param>().clone());
    } else {
      call_args.emplace_back(v_var.clone());
      invoke_params.emplace_back(p.as<op_func_param>().clone());
    }
  }

  auto call_f_lambda = VertexAdaptor<op_func_call>::create(call_args);
  call_f_lambda->str_val = f_lambda->name;
  call_f_lambda->func_id = f_lambda;

  auto invoke_cmd = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(call_f_lambda));
  auto invoke_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(invoke_params), invoke_cmd);

  FunctionPtr f_invoke = FunctionData::create_function(c_lambda->name + "$$" + "__invoke", invoke_root, FunctionData::func_local);
  f_invoke->outer_function = f_lambda;
  f_invoke->is_inline = true;
  f_invoke->return_typehint = f_lambda->return_typehint;
  f_invoke->has_variadic_param = f_lambda->has_variadic_param;
  f_invoke->root.set_location_recursively(Location(f_lambda->file_id, f_invoke, f_lambda->root->location.line));

  c_lambda->members.add_instance_method(f_invoke);

  return f_invoke;
}

// this function creates the __construct() method in a lambda class
//    function($x) use ($a)
// implicitly created a class Lambda$xxx { public $a; }, and here we generate
//    __construct($a) { $this->a = $a; }
static FunctionPtr generate_constructor_for_lambda_class(FunctionPtr f_lambda, ClassPtr c_lambda) {
  const Location &location = f_lambda->root->location;

  std::vector<VertexAdaptor<op_func_param>> construct_params;
  std::vector<VertexPtr> fields_initializers;
  for (auto var_as_use : f_lambda->uses_list) {
    auto var_as_param = VertexAdaptor<op_var>::create().set_location(location);
    var_as_param->str_val = var_as_use->extra_type == op_ex_var_this ? "captured$this" : var_as_use->str_val;
    construct_params.emplace_back(VertexAdaptor<op_func_param>::create(var_as_param));

    auto var_as_field = VertexAdaptor<op_instance_prop>::create(ClassData::gen_vertex_this(location)).set_location(location);
    var_as_field->str_val = var_as_use->str_val;
    fields_initializers.emplace_back(VertexAdaptor<op_set>::create(var_as_field, var_as_param.clone()).set_location(location));
  }
  auto construct_cmd = VertexAdaptor<op_seq>::create(fields_initializers);
  auto construct_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(construct_params), construct_cmd).set_location(location);

  c_lambda->create_constructor(construct_root);

  return c_lambda->construct_function;
}

// this function creates a lambda class
// with fields from uses_list, with __invoke() re-calling f_lambda and with __construct() initing fields
ClassPtr generate_lambda_class_wrapping_lambda_function(FunctionPtr f_lambda) {
  kphp_assert(f_lambda->is_lambda());

  ClassPtr c_lambda{new ClassData{ClassType::klass}};
  c_lambda->set_name_and_src_name("Lambda$" + f_lambda->name.substr(7));
  c_lambda->file_id = f_lambda->file_id;

  for (auto var_as_use : f_lambda->uses_list) {
    auto var_as_field = VertexAdaptor<op_var>::create().set_location(f_lambda->root);
    var_as_field->str_val = var_as_use->str_val;
    c_lambda->members.add_instance_field(var_as_field, {}, FieldModifiers{}.set_public(), nullptr, nullptr);

    kphp_error(!f_lambda->modifiers.is_static_lambda() || var_as_use->extra_type != op_ex_var_this,
               "Using $this in a static lambda");
  }

  FunctionPtr f_invoke = generate_invoke_method_for_lambda_class(f_lambda, c_lambda);
  FunctionPtr f_construct = generate_constructor_for_lambda_class(f_lambda, c_lambda);

  ++G->stats.total_lambdas;
  G->register_class(c_lambda);
  G->register_function(f_invoke);
  G->register_function(f_construct);

  kphp_assert(!f_lambda->class_id);
  f_lambda->class_id = c_lambda;    // save the class that wraps this lambda (its __invoke method calls it)

  return c_lambda;
}

// this function creates the __construct() invocation in a place where a lambda was used
//    $f = function($x) use($a) { ... };
// at first was converted to
//    $f = op_lambda;
// here we generate a vertex to replace:
//    $f = Lambda$xxx$$__construct(op_alloc, $a);
VertexPtr generate_lambda_class_construct_call_replacing_op_lambda(FunctionPtr f_lambda, VertexAdaptor<op_lambda> v_lambda) {
  std::vector<VertexPtr> construct_call_args;
  for (auto var_as_use : f_lambda->uses_list) {
    auto var_as_call_arg = var_as_use.clone().set_location(v_lambda->location);
    construct_call_args.emplace_back(var_as_call_arg);
  }

  ClassPtr c_lambda = f_lambda->class_id;
  return GenTree::gen_constructor_call_with_args(c_lambda, construct_call_args, v_lambda->location);
}

// having a typed callable, like "callable(int):void',
// we generate an interface, with the only method __invoke(int $arg1): void {}
// at the moment, it has no implementations:
// when actual lambdas are passed/assigned to this typed callable, they will implicitly implement this interface,
// and the body of __invoke() method will be generated like a regular virtual function, after processing all lambdas
InterfacePtr generate_interface_from_typed_callable(const TypeHint *type_hint) {
  const auto *typed_callable = type_hint->try_as<TypeHintCallable>();
  kphp_assert(typed_callable && typed_callable->is_typed_callable());

  std::string interface_name = "ITyped$" + replace_non_alphanum(typed_callable->as_human_readable());
  if (ClassPtr existing = G->get_class(interface_name)) {
    return existing;
  }

  InterfacePtr interface{new ClassData{ClassType::interface}};
  interface->modifiers.set_abstract();
  interface->set_name_and_src_name(interface_name);

  auto registered_interface = G->try_register_class(interface);
  if (registered_interface != interface) {
    while (!registered_interface->members.has_any_instance_method()) {    // __invoke not registered yet by another thread
      usleep(150);
    }
    return registered_interface;
  }

  std::vector<VertexAdaptor<op_func_param>> invoke_params;
  invoke_params.emplace_back(ClassData::gen_param_this({}));
  for (int i = 0; i < typed_callable->arg_types.size(); ++i) {
    auto v_param = VertexAdaptor<op_func_param>::create(VertexAdaptor<op_var>::create());
    v_param->var()->str_val = "arg" + std::to_string(i + 1);
    v_param->type_hint = typed_callable->arg_types[i];
    invoke_params.emplace_back(v_param);
  }

  auto invoke_cmd = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create());
  auto invoke_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(invoke_params), invoke_cmd);

  FunctionPtr f_invoke = FunctionData::create_function(interface_name + "$$" + "__invoke", invoke_root, FunctionData::func_local);
  f_invoke->is_virtual_method = true;
  f_invoke->return_typehint = typed_callable->return_type;
  f_invoke->root.set_location(Location{SrcFilePtr{}, f_invoke, -1});

  registered_interface->members.add_instance_method(f_invoke);
  G->register_function(f_invoke);

  return registered_interface;
}

// when a lambda function is used as a typed callable,
// make a lambda class implement a typed callable interface
void inherit_lambda_class_from_typed_callable(ClassPtr c_lambda, InterfacePtr typed_interface) {
  kphp_assert(c_lambda && c_lambda->is_lambda_class() && c_lambda->implements.empty());
  kphp_assert(typed_interface && typed_interface->is_typed_callable_interface());

  c_lambda->implements.emplace_back(typed_interface);

  AutoLocker<Lockable *> locker(&(*typed_interface));
  typed_interface->derived_classes.emplace_back(c_lambda);
}

// having 'strlen' or [$obj, 'method'] used as callable, generate a lambda
// think of it as: fn($s) => strlen($s) or function(...$args) use($obj) { return $obj->method(...$args); }
FunctionPtr generate_lambda_from_string_or_array(FunctionPtr current_function, const std::string &func_name, VertexPtr bound_this, VertexPtr v_location) {
  FunctionPtr f_to_be_wrapped;

  if (bound_this) {
    ClassPtr klass = assume_class_of_expr(current_function, bound_this, v_location).try_as_class();
    kphp_error_act(klass, fmt_format("Could not detect a class, method {} of which to call", func_name), return {});
    const auto *method = klass->get_instance_method(func_name);
    kphp_error_act(method, fmt_format("Could not find a method {} in class {}", func_name, klass->as_human_readable()), return {});
    f_to_be_wrapped = method->function;
  } else {
    f_to_be_wrapped = G->get_function(func_name);
  }

  kphp_error_act(f_to_be_wrapped, fmt_format("Could not find a function {}", func_name), return {});
  kphp_error_act(f_to_be_wrapped->is_required, fmt_format("{} needs the @kphp-required tag, because it's used only as a string callback", f_to_be_wrapped->as_human_readable()), return {});

  std::vector<VertexAdaptor<op_func_param>> w_params;
  std::vector<VertexPtr> w_call_args;
  for (auto p : f_to_be_wrapped->get_params()) {
    auto param = p.as<op_func_param>();

    if (param->extra_type == op_ex_param_variadic) {
      w_params.emplace_back(param.clone());
      w_call_args.emplace_back(VertexAdaptor<op_varg>::create(param->var().clone()));
    } else if (param->var()->extra_type != op_ex_var_this) {
      w_params.emplace_back(param.clone());
      w_call_args.emplace_back(param->var().clone());
    } else {
      w_call_args.emplace_back(bound_this);
    }
  }

  auto w_call = VertexAdaptor<op_func_call>::create(w_call_args);
  w_call->str_val = func_name;
  w_call->func_id = f_to_be_wrapped;
  auto w_cmd = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(w_call));

  auto w_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(w_params), w_cmd);

  FunctionPtr f_lambda = FunctionData::create_function(gen_anonymous_function_name(current_function), w_root, FunctionData::func_lambda);
  f_lambda->outer_function = current_function;
  f_lambda->return_typehint = f_to_be_wrapped->return_typehint;
  f_lambda->has_variadic_param = f_to_be_wrapped->has_variadic_param;
  f_lambda->root.set_location_recursively(Location(v_location->location.file, f_lambda, v_location->location.line));

  if (bound_this) {
    w_call->extra_type = op_ex_func_call_arrow;
    if (auto bound_this_var = bound_this.try_as<op_var>()) {    // [$inst, 'method'], not [getInst(), 'method']
      f_lambda->uses_list.emplace_front(bound_this_var);
    }
  }

  return f_lambda;
}

// having fn($x) => $x + $other + $captured + $variables,
// walk the arrow function body and capture them by value (fill f_lambda->uses_list)
void auto_capture_vars_from_body_in_arrow_lambda(FunctionPtr f_lambda) {
  auto needs_to_be_captured = [f_lambda](const std::string &var_name) {
    return var_name != "this" &&      // $this is captured by another approach, in non-arrow lambdas also
           var_name.find("::") == std::string::npos &&
           var_name.find("$u") == std::string::npos &&   // not a superlocal var created in gentree
           !VarData::does_name_eq_any_language_superglobal(var_name) &&
           !f_lambda->find_param_by_name(var_name) &&
           !f_lambda->find_use_by_name(var_name);
  };

  std::function<void(VertexPtr)> find_recursive = [&find_recursive, &needs_to_be_captured, f_lambda](VertexPtr root) {
    if (auto v = root.try_as<op_var>()) {
      if (needs_to_be_captured(v->str_val)) {
        f_lambda->uses_list.emplace_front(v);
      }
    } else if (auto inner_lambda = root.try_as<op_lambda>()) {
      for (auto v : inner_lambda->func_id->uses_list) {   // bubble up uses from nested lambdas
        if (needs_to_be_captured(v->str_val)) {
          f_lambda->uses_list.emplace_front(v);
        }
      }
    }

    for (auto v : *root) {
      find_recursive(v);
    }
  };

  find_recursive(f_lambda->root->cmd());
}
