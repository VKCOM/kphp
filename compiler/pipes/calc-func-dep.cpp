#include "compiler/pipes/calc-func-dep.h"

#include "compiler/data/var-data.h"

VertexPtr CalcFuncDepPass::on_enter_vertex(VertexPtr vertex, CalcFuncDepPass::LocalT *local) {
  if (local->extern_func_call && vertex->type() == op_func_ptr) {
    FunctionPtr callback_passed_to_extern_func = vertex->get_func_id();
    kphp_assert(callback_passed_to_extern_func);

    if (callback_passed_to_extern_func->is_lambda()) {
      /**
       * During code generation we replace constructor call in extern func_call with std::bind(LAMBDA$$__invoke, constructor_call, _1, _2, ...)
       * therefore no one know that outside function depends on LAMBDA$$__invoke method
       * this dependency need for generating #include directive for this method
       */
      data.dep.push_back(callback_passed_to_extern_func);
    }

    // There are 117 callbacks was passed to internal functions which throw exception
    //bool extern_func_throws_exception = local->extern_func_call->get_func_id()->can_throw;
    //bool callback_throws = !callback_passed_to_extern_func->can_throw;
    //if (callback_throws && !extern_func_throws_exception) {
    //  kphp_error(false, "It's not allowed to throw exception in callback which was passed to internal function");
    //}
  }

  //NB: There is no user functions in default values of any kind.
  if (vertex->type() == op_func_call) {
    VertexAdaptor<op_func_call> call = vertex;
    FunctionPtr other_function = call->get_func_id();
    data.dep.push_back(other_function);
    if (other_function->is_extern()) {
      local->extern_func_call = vertex.as<op_func_call>();
      return vertex;
    }

    int cnt_func_params = other_function->param_ids.size() - other_function->has_variadic_param;
    for (int ii = 0; ii < cnt_func_params; ++ii) {
      auto val = call->args()[ii];
      VarPtr to_var = other_function->param_ids[ii];
      if (to_var->is_reference) { //passed as reference
        while (val->type() == op_index) {
          val = val.as<op_index>()->array();
        }
        kphp_assert (val->type() == op_var || val->type() == op_instance_prop);
        VarPtr from_var = val->get_var_id();
        if (from_var->is_in_global_scope()) {
          data.global_ref_edges.emplace_back(from_var, to_var);
        } else if (from_var->is_reference) {
          data.ref_ref_edges.emplace_back(from_var, to_var);
        }
      }
      ii++;
    }
  } else if (vertex->type() == op_func_ptr) {
    data.dep.push_back(vertex.as<op_func_ptr>()->get_func_id());
  } else if (vertex->type() == op_constructor_call) {
    auto constructor_call = vertex.as<op_constructor_call>()->get_func_id();
    data.dep.push_back(constructor_call);
  } else if (vertex->type() == op_var/* && vertex->rl_type == val_l*/) {
    VarPtr var = vertex.as<op_var>()->get_var_id();
    if (var->is_in_global_scope()) {
      data.used_global_vars.push_back(var);
    }
  } else if (vertex->type() == op_func_param) {
    VarPtr var = vertex.as<op_func_param>()->var()->get_var_id();
    if (var->is_reference) {
      data.used_ref_vars.push_back(var);
    }
  } else if (vertex->type() == op_fork) {
    VertexPtr func_call = vertex.as<op_fork>()->func_call();
    kphp_error (func_call->type() == op_func_call, "Fork can be called only for function call.");
    data.forks.push_back(func_call->get_func_id());
  }

  return vertex;
}

DepData CalcFuncDepPass::on_finish() {
  my_unique(&data.dep);
  my_unique(&data.used_global_vars);
  return std::move(data);
}

bool CalcFuncDepPass::check_function(FunctionPtr function) {
  return default_check_function(function) && !function->is_extern();
}

void CalcFuncDepPass::on_enter_edge(VertexPtr, CalcFuncDepPass::LocalT *local, VertexPtr, CalcFuncDepPass::LocalT *dest_local) {
  dest_local->extern_func_call = local->extern_func_call;
}
