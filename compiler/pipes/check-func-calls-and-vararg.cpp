// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-func-calls-and-vararg.h"

#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/type-hint.h"

namespace {

std::optional<int64_t> get_const_int(VertexPtr expr) {
  if (auto as_int_conv = expr.try_as<op_conv_int>()) {
    expr = as_int_conv->expr();
  }
  expr = GenTree::get_actual_value(expr);
  if (expr->type() != op_int_const) {
    return std::nullopt;
  }
  return parse_int_from_string(expr.as<op_int_const>());
}

std::optional<bool> get_const_bool(VertexPtr expr) {
  if (auto as_bool_conv = expr.try_as<op_conv_bool>()) {
    expr = as_bool_conv->expr();
  }
  expr = GenTree::get_actual_value(expr);
  switch (expr->type()) {
    case op_true:
      return true;
    case op_false:
      return false;
    default:
      return std::nullopt;
  }
}

bool is_const_bool(VertexPtr expr, bool value) {
  auto cv = get_const_bool(expr);
  return cv.has_value() && cv.value() == value;
}

VertexAdaptor<op_func_call> replace_call_func(VertexAdaptor<op_func_call> call, const std::string &func_name, std::vector<VertexPtr> args) {
  auto new_call = VertexAdaptor<op_func_call>::create(std::move(args)).set_location(call);
  new_call->set_string(func_name);
  new_call->func_id = G->get_function(func_name);
  return new_call;
}

VertexAdaptor<op_func_call> process_preg_match(VertexAdaptor<op_func_call> call, bool all) {
  const auto &args = call->args();

  std::optional<int64_t> flags;
  if (args.size() >= 4) {
    flags = get_const_int(args[3]);
  }

  const char *func_name = all ? "preg_match_all_strings" : "preg_match_strings";

  // preg_match[_all]($pat, $s, $m)    -> preg_match[_all]_strings($pat, $s, $m)
  // preg_match[_all]($pat, $s, $m, 0) -> preg_match[_all]_strings($pat, $s, $m)
  if (args.size() == 3 || (args.size() == 4 && flags.value_or(-1) == 0)) {
    return replace_call_func(call, func_name, {args[0], args[1], args[2]});
  }

  // preg_match[_all]($pat, $s, $m, 0, $offset) -> preg_match[_all]_strings($pat, $s, $m, $offset)
  if (args.size() == 5 && flags.value_or(-1) == 0) {
    return replace_call_func(call, func_name, {args[0], args[1], args[2], args[4]});
  }

  return call;
}

VertexAdaptor<op_func_call> process_microtime(VertexAdaptor<op_func_call> call) {
  const auto &args = call->args();

  // microtime()      -> microtime_string()
  // microtime(false) -> microtime_string()
  if (args.empty() || is_const_bool(args[0], false)) {
    return replace_call_func(call, "microtime_string", {});
  }

  // microtime(true) -> microtime_float()
  if (!args.empty() && is_const_bool(args[0], true)) {
    return replace_call_func(call, "microtime_float", {});
  }

  return call;
}

VertexAdaptor<op_func_call> process_hrtime(VertexAdaptor<op_func_call> call) {
  const auto &args = call->args();

  // hrtime()      -> hrtime_array()
  // hrtime(false) -> hrtime_array()
  if (args.empty() || is_const_bool(args[0], false)) {
    return replace_call_func(call, "hrtime_array", {});
  }

  // hrtime(true) -> hrtime_int()
  if (!args.empty() && is_const_bool(args[0], true)) {
    return replace_call_func(call, "hrtime_int", {});
  }

  return call;
}

} // namespace

/*
 * This pass checks that func calls are correct, i.e. provided necessary amount of arguments, etc.
 * Here we also replace ...variadic call arguments with a single array
 * (we do this after the previous pipe: after `$f()` becomes `$f->__invoke()`, as an invocation may also be variadic).
 */

VertexPtr CheckFuncCallsAndVarargPass::on_enter_vertex(VertexPtr root) {
  if (root->type() == op_func_call) {
    return on_func_call(root.as<op_func_call>());
  } else if (root->type() == op_fork) {
    return on_fork(root.as<op_fork>());
  }

  return root;
}

// calling a function with variadic arguments involves some transformations:
//   function fun($x, ...$args)
// transformations will be:
//   fun(1, 2, 3) -> fun(1, [2, 3])
//   fun(1, ...$arr) -> fun(1, $arr)
//   fun(1, 2, 3, ...$arr1, ...$arr2) -> fun(1, array_merge([2, 3], $arr1, $arr2))
// if this function is method of some class, than we need skip implicit $this argument
//   $this->fun(1, 2, 3) -> fun($this, 1, [2, 3])
// this is done here (not in deducing types), as $f(...$variadic) — in invoke, not func call — are applicable only here
VertexAdaptor<op_func_call> CheckFuncCallsAndVarargPass::process_varargs(VertexAdaptor<op_func_call> call, FunctionPtr f_called) {
  const std::vector<VertexPtr> &cur_call_args = call->get_next();
  auto positional_args_start = cur_call_args.begin();
  auto variadic_args_start = positional_args_start;

  kphp_error_act(f_called->get_params().size() - 1 <= cur_call_args.size(),
                 "Not enough arguments to supply a variadic call",
                 return call);
  std::advance(variadic_args_start, f_called->get_params().size() - 1);

  auto unpacking_args_start = std::find_if(cur_call_args.begin(), cur_call_args.end(), [](VertexPtr v) { return v->type() == op_varg; });
  kphp_error_act(std::distance(variadic_args_start, unpacking_args_start) >= 0,
                 "It's prohibited to unpack arrays in places where positional arguments expected",
                 return call);
  std::vector<VertexPtr> new_call_args(positional_args_start, variadic_args_start);

  // case when variadic params just have been forwarded
  // e.g. fun(...$args) will be transformed to f($args) without any array_merge
  if (variadic_args_start == unpacking_args_start && std::distance(unpacking_args_start, cur_call_args.end()) == 1) {
    new_call_args.emplace_back(GenTree::conv_to<tp_array>(unpacking_args_start->as<op_varg>()->array()));
  } else {
    std::vector<VertexPtr> variadic_args_passed_as_positional(variadic_args_start, unpacking_args_start);
    auto array_from_varargs_passed_as_positional = VertexAdaptor<op_array>::create(variadic_args_passed_as_positional);
    array_from_varargs_passed_as_positional.set_location(call);

    if (unpacking_args_start != cur_call_args.end()) {
      std::vector<VertexPtr> unpacking_args_converted_to_array;
      unpacking_args_converted_to_array.reserve(static_cast<size_t>(std::distance(unpacking_args_start, cur_call_args.end())));
      for (auto unpack_arg_it = unpacking_args_start; unpack_arg_it != cur_call_args.end(); ++unpack_arg_it) {
        if (auto unpack_as_varg = unpack_arg_it->try_as<op_varg>()) {
          unpacking_args_converted_to_array.emplace_back(GenTree::conv_to<tp_array>(unpack_as_varg->array()));
        } else {
          const std::string &s = (*unpack_arg_it)->has_get_string() ? (*unpack_arg_it)->get_string() : "";
          kphp_error_act(false, fmt_format("It's prohibited using something after argument unpacking: `{}`", s), return call);
        }
      }
      auto merge_arrays = VertexAdaptor<op_func_call>::create(array_from_varargs_passed_as_positional, unpacking_args_converted_to_array).set_location(call);
      merge_arrays->str_val = "array_merge";
      merge_arrays->func_id = G->get_function(merge_arrays->str_val);

      new_call_args.emplace_back(merge_arrays);
    } else {
      new_call_args.emplace_back(array_from_varargs_passed_as_positional);
    }
  }

  auto new_call = VertexAdaptor<op_func_call>::create(new_call_args).set_location(call);
  new_call->extra_type = call->extra_type;
  new_call->str_val = call->str_val;
  new_call->func_id = f_called;

  return new_call;
}

// when calling f($arg) like f() (without $arg), then maybe, this $arg is auto-filled
VertexPtr CheckFuncCallsAndVarargPass::maybe_autofill_missing_call_arg(VertexAdaptor<op_func_call> call, [[maybe_unused]] FunctionPtr f_called, VertexAdaptor<op_func_param> param) {
  if (param->type_hint == nullptr) {
    return {};
  }

  // CompileTimeLocation is a built-in KPHP class
  // in PHP, we declare f(CompileTimeLocation $loc = null) and just call f()
  // in KPHP, we implicitly replace f() with f(new CompileTimeLocation(__FILE__, __METHOD__, __LINE__))
  if (const auto *as_instance = param->type_hint->unwrap_optional()->try_as<TypeHintInstance>()) {
    if (as_instance->resolve()->name == "CompileTimeLocation") {
      return CheckFuncCallsAndVarargPass::create_CompileTimeLocation_call_arg(call->location);
    }
  }

  return {};
}

// create constructor call: new CompileTimeLocation(__RELATIVE_FILE__, __METHOD__, __LINE__)
VertexPtr CheckFuncCallsAndVarargPass::create_CompileTimeLocation_call_arg(const Location &call_location) {
  ClassPtr klass_CompileTimeLocation = G->get_class("CompileTimeLocation");
  kphp_assert(klass_CompileTimeLocation);

  auto v_file = VertexAdaptor<op_string>::create().set_location(call_location);
  v_file->str_val = call_location.get_file()->relative_file_name;

  auto v_function = VertexAdaptor<op_string>::create().set_location(call_location);
  if (current_function->is_lambda()) {  // like tok_method_c in gentree (PHP polyfill also emits the same format)
    v_function->str_val = "{closure}";
  } else if (!current_function->is_main_function()) {
    v_function->str_val = !current_function->modifiers.is_nonmember()
                          ? current_function->class_id->name + "::" + current_function->local_name()
                          : current_function->name;
  }

  auto v_line = GenTree::create_int_const(call_location.get_line());

  auto v_alloc = VertexAdaptor<op_alloc>::create().set_location(call_location);
  v_alloc->allocated_class = klass_CompileTimeLocation;

  auto v_loc = VertexAdaptor<op_func_call>::create(v_alloc, v_file, v_function, v_line).set_location(call_location);
  v_loc->func_id = klass_CompileTimeLocation->members.get_instance_method("__construct")->function;
  v_loc->extra_type = op_ex_constructor_call;
  return v_loc;
}

VertexAdaptor<op_func_call> CheckFuncCallsAndVarargPass::maybe_replace_extern_func_call(VertexAdaptor<op_func_call> call, FunctionPtr f_called) {
  vk::string_view name = f_called->name;
  if (name == "hrtime") {
    return process_hrtime(call);
  }
  if (name == "microtime") {
    return process_microtime(call);
  }
  if (name == "preg_match") {
    return process_preg_match(call, false);
  }
  if (name == "preg_match_all") {
    return process_preg_match(call, true);
  }
  return call;
}

VertexAdaptor<op_func_call> CheckFuncCallsAndVarargPass::add_call_arg(VertexPtr to_add, VertexAdaptor<op_func_call> call, bool prepend) {
  std::vector<VertexPtr> new_args;
  new_args.reserve(call->args().size() + 1);
  if (prepend) {
    new_args.emplace_back(to_add);
  }
  for (auto arg : call->args()) {
    new_args.emplace_back(arg);
  }
  if (!prepend) {
    new_args.emplace_back(to_add);
  }

  auto new_call = VertexAdaptor<op_func_call>::create(new_args).set_location(call->location);
  new_call->str_val = call->str_val;
  new_call->func_id = call->func_id;
  return new_call;
}

// check func call, that a call is valid (not abstract, etc) and provided a valid number of arguments
// also, check the number of arguments passed as callbacks to extern functions
VertexPtr CheckFuncCallsAndVarargPass::on_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr f = call->func_id;
  if (!f) {
    return call;    // an error has already been printed when it wasn't set
  }

  if (f->has_variadic_param) {
    call = process_varargs(call, call->func_id);
  }

  if (f->modifiers.is_static()) {
    kphp_error(call->extra_type != op_ex_func_call_arrow,
               fmt_format("Called static method {}() using -> (need to use ::)", f->as_human_readable()));
  }
  if (f->modifiers.is_instance()) {
    kphp_error(call->extra_type == op_ex_func_call_arrow || call->extra_type == op_ex_constructor_call,
               fmt_format("Non-static method {}() is called statically (use ->, not ::)", f->as_human_readable()));
  }
  if (f->modifiers.is_abstract()) {
    kphp_error(f->is_virtual_method,
               fmt_format("Can not call an abstract method {}()", f->as_human_readable()));
  }

  VertexRange func_params = f->get_params();
  VertexRange call_params = call->args();

  if (call_params.size() < func_params.size()) {
    for (int i = call_params.size(); i < func_params.size(); ++i) {
      if (VertexPtr auto_added = maybe_autofill_missing_call_arg(call, f, func_params[i].as<op_func_param>())) {
        call = add_call_arg(auto_added, call, false);
        call_params = call->args();
      }
    }
  }

  int call_n_params = call_params.size();
  int delta_this = f->has_implicit_this_arg() ? 1 : 0;    // not to count implicit $this on error output

  kphp_error(call_n_params >= f->get_min_argn(),
             fmt_format("Too few arguments to function call, expected {}, have {}", f->get_min_argn() - delta_this, call_n_params - delta_this));
  kphp_error(func_params.size() >= call_n_params,
             fmt_format("Too many arguments to function call, expected {}, have {}", func_params.size() - delta_this, call_n_params - delta_this));

  for (int i = 0; i < call_params.size() && i < func_params.size(); ++i) {
    auto func_param = func_params[i].as<op_func_param>();
    auto call_arg = call_params[i];

    if (f->is_extern() && func_param->type_hint && call_arg->type() == op_callback_of_builtin) {
      if (FunctionPtr f_callback = call_arg.as<op_callback_of_builtin>()->func_id) {
        int call_n_params = func_param->type_hint->try_as<TypeHintCallable>()->arg_types.size() + call_arg.as<op_callback_of_builtin>()->args().size();
        int delta_this = f_callback->has_implicit_this_arg() ? 1 : 0;
        kphp_error(call_n_params >= f_callback->get_min_argn(),
                   fmt_format("Too few arguments in callback, expected {}, have {}", f_callback->get_min_argn() - delta_this, call_n_params - delta_this));
        kphp_error(f_callback->get_params().size() >= call_n_params,
                   fmt_format("Too many arguments in callback, expected {}, have {}", f_callback->get_params().size() - delta_this, call_n_params - delta_this));

        for (auto p : f_callback->get_params()) {
          kphp_error(!p.as<op_func_param>()->var()->ref_flag, "You can't pass callbacks with &references to built-in functions");
        }
      }
    }

    if (call_arg->type() == op_varg) {
      kphp_error(f->has_variadic_param, "Unpacking an argument to a non-variadic param");
    } else if (auto conv = call_arg.try_as<op_conv_array>(); conv && conv->expr()->type() == op_varg) {
      kphp_error(f->has_variadic_param, "Unpacking an argument to a non-variadic param");
    }
  }

  if (f->is_extern() && !stage::has_error()) {
    call = maybe_replace_extern_func_call(call, call->func_id);
  }

  return call;
}

// check that fork(...) is called correctly, as fork(f())
// note, that we do this after replacing op_invoke_call with op_func_call, not earlier
VertexPtr CheckFuncCallsAndVarargPass::on_fork(VertexAdaptor<op_fork> v_fork) {
  kphp_error(v_fork->size() == 1 && (*v_fork->begin())->type() == op_func_call,
             "Invalid fork() usage: it must be called with exactly one func call inside, e.g. fork(f(...))");
  return v_fork;
}
