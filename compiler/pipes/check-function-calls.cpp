#include "compiler/pipes/check-function-calls.h"

#include "compiler/data/src-file.h"

void CheckFunctionCallsPass::check_func_call(VertexPtr call) {
  FunctionPtr f = call->get_func_id();
  kphp_assert(f);
  kphp_error_return(f->root, dl_pstr("Function [%s] undeclared", f->name.c_str()));

  if (call->type() == op_func_ptr || f->varg_flag) {
    return;
  }

  VertexRange func_params = f->root.as<meta_op_function>()->params().as<op_func_param_list>()->params();

  VertexRange call_params = call->type() == op_constructor_call ? call.as<op_constructor_call>()->args()
                                                                : call.as<op_func_call>()->args();
  int func_params_n = static_cast<int>(func_params.size());
  int call_params_n = static_cast<int>(call_params.size());

  kphp_error_return(call_params_n >= f->min_argn,
                    dl_pstr("Not enough arguments in function [%s:%s] [found %d] [expected at least %d]",
                            f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, f->min_argn)
  );

  kphp_error(call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
             dl_pstr("call_user_func_array is used for function [%s:%s]",
                     f->file_id->file_name.c_str(), f->name.c_str()
             )
  );

  kphp_error_return(func_params_n >= call_params_n,
                    dl_pstr("Too much arguments in function [%s:%s] [found %d] [expected %d]",
                            f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, func_params_n
                    )
  );
}
VertexPtr CheckFunctionCallsPass::on_enter_vertex(VertexPtr v, LocalT*) {
  if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
    check_func_call(v);
  }
  return v;
}
