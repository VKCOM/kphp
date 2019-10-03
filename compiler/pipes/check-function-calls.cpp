#include "compiler/pipes/check-function-calls.h"

#include "compiler/data/src-file.h"

void CheckFunctionCallsPass::check_func_call(VertexPtr call) {
  FunctionPtr f = call->type() == op_func_ptr ? call.as<op_func_ptr>()->func_id : call.as<op_func_call>()->func_id;
  kphp_assert(f);
  kphp_error_return(f->root, format("Function [%s] undeclared", f->get_human_readable_name().c_str()));

  if (f->modifiers.is_static()) {
    kphp_error_return(call->extra_type != op_ex_func_call_arrow,
                      format("Called static method %s() using -> (need to use ::)", f->get_human_readable_name().c_str()));
  }
  if (f->modifiers.is_instance() && !f->is_constructor() && !f->is_lambda()) {
    kphp_error_return(call->extra_type == op_ex_func_call_arrow,
                      format("Called instance method %s() using :: (need to use ->)", f->get_human_readable_name().c_str()));
  }

  if (call->type() == op_func_ptr || f->is_vararg) {
    return;
  }

  VertexRange func_params = f->get_params();
  VertexRange call_params = call.as<op_func_call>()->args();

  int func_params_n = static_cast<int>(func_params.size());
  int call_params_n = static_cast<int>(call_params.size());

  kphp_error_return(call_params_n >= f->get_min_argn(),
                    format("Not enough arguments in function call [%s : %s] [found %d] [expected at least %d]",
                            f->file_id->file_name.c_str(), f->get_human_readable_name().c_str(), call_params_n, f->get_min_argn())
  );

  kphp_error(call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
             format("call_user_func_array is used for function [%s : %s]",
                     f->file_id->file_name.c_str(), f->get_human_readable_name().c_str()
             )
  );

  kphp_error_return(func_params_n >= call_params_n,
                    format("Too much arguments in function call [%s : %s] [found %d] [expected %d]",
                            f->file_id->file_name.c_str(), f->get_human_readable_name().c_str(), call_params_n, func_params_n
                    )
  );

  for (int i = 0; i < call_params.size(); i++) {
    if (auto func_param = func_params[i].try_as<op_func_param_callback>()) {
      auto call_param = call_params[i].try_as<op_func_ptr>();
      kphp_error_return(call_param,
                        fmt_format("Argument '{}' should be function pointer, but {} found [{} : {}]",
                                   func_param->var()->get_c_string(),
                                   OpInfo::str(call_params[i]->type()),
                                   f->file_id->file_name, f->get_human_readable_name()
                        ));

      if (!FunctionData::check_cnt_params(get_function_params(func_param).size(), call_param->func_id)) {
        return;
      }
    }
  }
}
VertexPtr CheckFunctionCallsPass::on_enter_vertex(VertexPtr v, LocalT*) {
  if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
    check_func_call(v);
  }
  return v;
}
