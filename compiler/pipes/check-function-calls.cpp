#include "compiler/pipes/check-function-calls.h"

#include "compiler/data/src-file.h"

void CheckFunctionCallsPass::check_func_call(VertexPtr call) {
  FunctionPtr f = call->get_func_id();
  kphp_assert(f);
  kphp_error_return(f->root, format("Function [%s] undeclared", f->get_human_readable_name().c_str()));

  if (f->is_static_function()) {
    kphp_error_return(call->extra_type != op_ex_func_call_arrow,
                      format("Called static method %s() using -> (need to use ::)", f->get_human_readable_name().c_str()));
  }
  if (f->is_instance_function() && !f->is_constructor() && !f->is_lambda()) {
    kphp_error_return(call->extra_type == op_ex_func_call_arrow,
                      format("Called instance method %s() using :: (need to use ->)", f->get_human_readable_name().c_str()));
  }

  if (call->type() == op_func_ptr || f->varg_flag) {
    return;
  }

  VertexRange func_params = f->root.as<meta_op_function>()->params().as<op_func_param_list>()->params();

  VertexRange call_params = call.as<op_func_call>()->args();
  int func_params_n = static_cast<int>(func_params.size());
  int call_params_n = static_cast<int>(call_params.size());

  kphp_error_return(call_params_n >= f->min_argn,
                    format("Not enough arguments in function [%s : %s] [found %d] [expected at least %d]",
                            f->file_id->file_name.c_str(), f->get_human_readable_name().c_str(), call_params_n, f->min_argn)
  );

  kphp_error(call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
             format("call_user_func_array is used for function [%s : %s]",
                     f->file_id->file_name.c_str(), f->get_human_readable_name().c_str()
             )
  );

  kphp_error_return(func_params_n >= call_params_n,
                    format("Too much arguments in function [%s : %s] [found %d] [expected %d]",
                            f->file_id->file_name.c_str(), f->get_human_readable_name().c_str(), call_params_n, func_params_n
                    )
  );

  for (int i = 0; i < call_params.size(); i++) {
    if (func_params[i]->type() == op_func_param_callback) {
      kphp_error_return(call_params[i]->type() == op_func_ptr,
                        format("Argument '%s' should be function pointer, but %s found [%s : %s]",
                                func_params[i].as<op_func_param_callback>()->var()->get_c_string(),
                                OpInfo::str(call_params[i]->type()).c_str(),
                                f->file_id->file_name.c_str(), f->get_human_readable_name().c_str()
                        ));
    }
  }
}
VertexPtr CheckFunctionCallsPass::on_enter_vertex(VertexPtr v, LocalT*) {
  if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
    check_func_call(v);
  }
  return v;
}
