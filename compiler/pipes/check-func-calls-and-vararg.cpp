// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-func-calls-and-vararg.h"

#include "compiler/gentree.h"
#include "compiler/name-gen.h"
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
