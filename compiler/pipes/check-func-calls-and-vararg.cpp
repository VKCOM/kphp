// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-func-calls-and-vararg.h"

#include "compiler/modulite-check-rules.h"
#include "compiler/data/src-file.h"
#include "compiler/rewrite-rules/replace-extern-func-calls.h"
#include "compiler/vertex-util.h"
#include "compiler/type-hint.h"

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
// here we also deal with unpacking fixed-size arrays into positional arguments:
//   fun(...[1]) -> fun(1)
//   fun(...[1,2]) -> fun(1, [2])
//   fun(...[1, ...[2, ...[3, ...$rest]]]) => fun(1, array_merge([2,3], $rest))
// this is done here (not in deducing types), as $f(...$variadic) — in invoke, not func call — are applicable only here
VertexAdaptor<op_func_call> CheckFuncCallsAndVarargPass::process_varargs(VertexAdaptor<op_func_call> call, FunctionPtr f_called) {
  std::vector<VertexPtr> flattened_call_args;
  flattened_call_args.reserve(call->args().size());
  VertexRange f_params = f_called->get_params();
  VertexRange call_args = call->args();

  // at first, convert f(1, ...[2, ...[3]], ...$all, ...[5]) to f(1,2,3,...$all,5)
  std::function<void(VertexPtr)> flatten_call_varg = [&flattened_call_args, &flatten_call_varg](VertexPtr inner) {
    if (auto as_array = inner.try_as<op_array>()) {
      for (VertexPtr item : as_array->args()) {
        kphp_error(item->type() != op_double_arrow, "Passed unpacked ...[array] must be a vector, without => keys");
        kphp_assert(item->type() != op_varg);
        flattened_call_args.emplace_back(item);
      }
    } else if (auto as_merge = inner.try_as<op_func_call>(); as_merge && as_merge->str_val == "array_merge_spread") {
      for (VertexPtr item : as_merge->args()) {
        kphp_assert(item->type() == op_conv_array);
        flatten_call_varg(item.as<op_conv_array>()->expr());
      }
    } else {
      auto wrap_varg = VertexAdaptor<op_varg>::create(inner).set_location(inner);
      flattened_call_args.emplace_back(wrap_varg);
    }
  };

  for (VertexPtr call_arg : call->args()) {
    if (auto as_varg = call_arg.try_as<op_varg>()) {
      size_t n_before = flattened_call_args.size();
      flatten_call_varg(as_varg->array());
      for (size_t i = n_before; i <= flattened_call_args.size() && i < f_params.size(); ++i) {
        auto ith_param = f_params[i].as<op_func_param>();
        kphp_error(!ith_param->is_cast_param, fmt_format("Invalid place for unpack, because param ${} is @kphp-infer cast", ith_param->var()->str_val));
      }
    } else {
      flattened_call_args.emplace_back(call_arg);
    }
  }

  std::vector<VertexPtr> new_call_args;
  bool is_just_single_arg_forwarded = false;
  bool needs_wrap_array_merge = false;
  std::vector<VertexPtr> variadic_args_passed;
  int i_func_param = 0;

  // then, having f($x,$y,...$rest) and a call f(1,2,3,...$all,5), detect that variadic_args_passed = [3,...$all,5]
  for (int i_call_arg = 0; i_call_arg < flattened_call_args.size(); ++i_call_arg) {
    VertexPtr ith_call_arg = flattened_call_args[i_call_arg];
    bool is_variadic_param = f_called->has_variadic_param && i_func_param == f_params.size() - 1;

    if (!is_variadic_param) {
      i_func_param++;
      new_call_args.emplace_back(ith_call_arg);
      kphp_error(ith_call_arg->type() != op_varg, "It's prohibited to unpack non-fixed arrays where positional arguments expected");
      continue;
    }

    if (auto unpack_as_varg = ith_call_arg.try_as<op_varg>()) {
      if (i_call_arg == call_args.size() - 1 && i_func_param == f_params.size() - 1 && variadic_args_passed.empty()) {
        // variadic just have been forwarded, e.g. f(...$args) transformed to f($args) without any array_merge
        variadic_args_passed.emplace_back(VertexUtil::create_conv_to(tp_array, unpack_as_varg->array()));
        is_just_single_arg_forwarded = true;
      } else {
        // f(...$args, ...$another) transformed to f(array_merge($args, $another))
        variadic_args_passed.emplace_back(VertexUtil::create_conv_to(tp_array, unpack_as_varg->array()));
        needs_wrap_array_merge = true;
      }
    } else {
      variadic_args_passed.emplace_back(ith_call_arg);
    }
  }

  // append $rest = (from variadic_args_passed) as the last new_call_args
  if (f_called->has_variadic_param && i_func_param == f_params.size() - 1) {
    if (is_just_single_arg_forwarded) {
      // optimization: f(...$args) transformed to f($args) without any array_merge
      kphp_assert(variadic_args_passed.size() == 1);
      new_call_args.emplace_back(variadic_args_passed.front());

    } else if (!needs_wrap_array_merge) {
      // f(1, 2, 3) transformed into f([1,2,3])
      auto rest_array = VertexAdaptor<op_array>::create(variadic_args_passed).set_location(call);
      new_call_args.emplace_back(rest_array);

    } else {
      // f(1, $v, ...$args, ...get_more()) transformed into f(array_merge([1,$v], $args, get_more()))
      std::vector<VertexPtr> variadic_args_conv_array;
      for (int i = 0; i < variadic_args_passed.size(); ++i) {
        if (variadic_args_passed[i]->type() == op_conv_array) {
          variadic_args_conv_array.emplace_back(variadic_args_passed[i]);
          continue;
        }

        // group a sequence "1, $v" into a single array [1,$v]
        std::vector<VertexPtr> seq_items;
        for (; i < variadic_args_passed.size(); ++i) {
          if (variadic_args_passed[i]->type() == op_conv_array) {
            i--;
            break;
          }
          seq_items.emplace_back(variadic_args_passed[i]);
        }
        auto seq_array = VertexAdaptor<op_array>::create(seq_items).set_location(call);
        variadic_args_conv_array.emplace_back(seq_array);
      }

      auto merge_arrays = VertexAdaptor<op_func_call>::create(variadic_args_conv_array).set_location(call);
      merge_arrays->str_val = "array_merge";
      merge_arrays->func_id = G->get_function(merge_arrays->str_val);
      new_call_args.emplace_back(merge_arrays);
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
    if (as_instance->resolve()->name == "CompileTimeLocation" && param->has_default_value()) {
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

  auto v_line = VertexUtil::create_int_const(call_location.get_line());

  auto v_alloc = VertexAdaptor<op_alloc>::create().set_location(call_location);
  v_alloc->allocated_class = klass_CompileTimeLocation;

  auto v_loc = VertexAdaptor<op_func_call>::create(v_alloc, v_file, v_function, v_line).set_location(call_location);
  v_loc->func_id = klass_CompileTimeLocation->members.get_instance_method("__construct")->function;
  v_loc->extra_type = op_ex_constructor_call;
  return v_loc;
}


// check func call, that a call is valid (not abstract, etc) and provided a valid number of arguments
// also, check the number of arguments passed as callbacks to extern functions
VertexPtr CheckFuncCallsAndVarargPass::on_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr f = call->func_id;
  if (!f) {
    return call;    // an error has already been printed when it wasn't set
  }

  if (f->is_extern()) {
    if (f->is_internal) {
      kphp_error_act(call->auto_inserted, fmt_format("Called internal function {}()", f->as_human_readable()), return call);
    }
    // probably, replace f(...) with f'(...) or even with a non-call expr
    // for instance, `_by_name_get_const('A', 'C')` is replaced with value of A::C
    // for instance, `MyJsonEncoder::encode(...)` is replaced with `JsonEncoder::to_json_impl('MyJsonEncoder', ...)`
    VertexPtr repl = maybe_replace_extern_func_call(current_function, call);
    auto repl_with_call = repl.try_as<op_func_call>();
    if (!repl_with_call) {
      return repl;
    }
    call = repl_with_call;
    f = call->func_id;
  }

  // if f_called is f(...$args) or call is f(...[1,2]), then resample called arguments
  bool has_variadic_arg = vk::any_of(call->args(), [](VertexPtr arg) {
    return arg->type() == op_varg;
  });
  if (f->has_variadic_param || has_variadic_arg) {
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
    for (int missing_i = call_params.size(); missing_i < func_params.size(); ++missing_i) {
      if (VertexPtr auto_added = maybe_autofill_missing_call_arg(call, f, func_params[missing_i].as<op_func_param>())) {
        for (int def_i = call_params.size(); def_i < missing_i; ++def_i) {
          call = VertexUtil::add_call_arg(func_params[def_i].as<op_func_param>()->default_value(), call, false);
        }
        call = VertexUtil::add_call_arg(auto_added, call, false);
        call_params = call->args();
      }
    }
  }

  int call_n_params = call_params.size();
  int delta_this = f->has_implicit_this_arg() ? 1 : 0;    // not to count implicit $this on error output

  kphp_error(call_n_params >= f->get_min_argn(),
             fmt_format("Too few arguments in call to {}(), expected {}, have {}", TermStringFormat::paint_bold(f->as_human_readable()), f->get_min_argn() - delta_this, call_n_params - delta_this));
  kphp_error(func_params.size() >= call_n_params,
             fmt_format("Too many arguments in call to {}(), expected {}, have {}", TermStringFormat::paint_bold(f->as_human_readable()), func_params.size() - delta_this, call_n_params - delta_this));

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
  }

  if (current_function->modulite != f->modulite) {
    bool is_instance_call = f->modifiers.is_instance();
    bool is_constructor_call = f->is_constructor() && !f->class_id->is_lambda_class();
    if (f->type == FunctionData::func_local && !is_instance_call) {
      modulite_check_when_call_function(current_function, f);
    } else if (f->type == FunctionData::func_local && is_constructor_call) {
      modulite_check_when_use_class(current_function, f->class_id);
    }
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
