// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-function-calls.h"

#include "compiler/data/src-file.h"
#include "compiler/type-hint.h"

VertexRange CheckFunctionCallsPass::process_named_args(FunctionPtr func, VertexAdaptor<op_func_call> call) {
  const auto args = call->args();
  auto see_named = false;

  const auto without_named_args = std::none_of(args.begin(), args.end(), [&see_named](const VertexPtr arg) -> bool {
    const auto named = arg->type() == op_named_arg;
    if (named) {
      see_named = true;
    }
    // if positional arg after named
    if (!named && see_named) {
      kphp_error(0, "Cannot use positional argument after named argument");
    }
    return named;
  });

  if (without_named_args) {
    return call->args();
  }

  const auto params = func->get_params();
  auto params_map = std::unordered_map<std::string, size_t>(params.size());

  for (int i = 0; i < params.size(); ++i) {
    const auto &param = params[i].as<op_func_param>();
    params_map[param->var()->str_val] = i;
  }

  auto new_call_args = std::vector<VertexPtr>(params.size());

  for (const auto &arg : args) {
    if (const auto named = arg.try_as<op_named_arg>()) {
      const auto name = named->name()->str_val;
      if (params_map.find(name) == params_map.end()) {
        kphp_error(0, fmt_format("Unknown named parameter {}", name));
        continue;
      }

      const auto param_index = params_map[name];
      new_call_args[param_index] = named;
    }
  }

  auto end = Vertex::iterator{&new_call_args.front()};
  auto begin = Vertex::iterator{&new_call_args.back()};

  return {begin, end + 1};
}

void CheckFunctionCallsPass::check_func_call(VertexPtr call) {
  FunctionPtr f = call->type() == op_func_ptr ? call.as<op_func_ptr>()->func_id : call.as<op_func_call>()->func_id;
  kphp_assert(f);
  kphp_error_return(f->root, fmt_format("Function [{}] undeclared", f->get_human_readable_name()));

  call->throw_flag = f->can_throw();
  if (f->modifiers.is_static()) {
    kphp_error_return(call->extra_type != op_ex_func_call_arrow,
                      fmt_format("Called static method {}() using -> (need to use ::)", f->get_human_readable_name()));
  }
  if (f->modifiers.is_instance() && !f->is_constructor() && !f->is_lambda()) {
    kphp_error_return(call->extra_type == op_ex_func_call_arrow,
                      fmt_format("Called instance method {}() using :: (need to use ->)", f->get_human_readable_name()));
  }

  if (call->type() == op_func_ptr) {
    return;
  }

  VertexRange func_params = f->get_params();
  VertexRange call_params = process_named_args(f, call.as<op_func_call>());

  auto actual_params_n = [f](int n) {
    // instance methods always have implicit $this param which we don't want to mention
    return f->modifiers.is_instance() ? n - 1 : n;
  };

  int func_params_n = static_cast<int>(func_params.size());
  int call_params_n = static_cast<int>(call_params.size());
  kphp_error_return(call_params_n >= f->get_min_argn(),
                    fmt_format("Not enough arguments in function call [{} : {}] [found {}] [expected at least {}]",
                            f->file_id->file_name, f->get_human_readable_name(), actual_params_n(call_params_n), actual_params_n(f->get_min_argn()))
  );

  kphp_error(call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
             fmt_format("call_user_func_array is used for function [{} : {}]", f->file_id->file_name, f->get_human_readable_name())
  );

  kphp_error_return(func_params_n >= call_params_n,
                    fmt_format("Too much arguments in function call [{} : {}] [found {}] [expected {}]",
                               f->file_id->file_name, f->get_human_readable_name(), actual_params_n(call_params_n), actual_params_n(func_params_n))
  );

  for (int i = 0; i < call_params.size(); i++) {
    auto func_param = func_params[i].as<op_func_param>();
    if (f->is_extern() && func_param->type_hint && func_param->type_hint->try_as<TypeHintCallable>()) {
      auto call_param = call_params[i].try_as<op_func_ptr>();
      kphp_error_return(call_param,
                        fmt_format("Argument '{}' should be function pointer, but {} found",
                                   func_param->var()->get_string(),
                                   OpInfo::str(call_params[i]->type())));

      if (!FunctionData::check_cnt_params(func_param->type_hint->try_as<TypeHintCallable>()->arg_types.size(), call_param->func_id)) {
        return;
      }
    }
  }
}

VertexPtr CheckFunctionCallsPass::on_enter_vertex(VertexPtr v) {
  if (v->type() == op_func_ptr || v->type() == op_func_call) {
    check_func_call(v);
  }
  return v;
}

VertexPtr CheckFunctionCallsPass::on_exit_vertex(VertexPtr v) {
  for (auto child : *v) {
    if (child->throw_flag) {
      v->throw_flag = true;
      break;
    }
  }
  return v;
}
