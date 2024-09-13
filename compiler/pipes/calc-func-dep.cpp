// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-func-dep.h"

#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/vertex.h"

VertexPtr CalcFuncDepPass::on_enter_vertex(VertexPtr vertex) {
  if (!calls.empty() && calls.back()->is_extern() && vertex->type() == op_callback_of_builtin) {
    FunctionPtr callback_passed_to_extern_func = vertex.as<op_callback_of_builtin>()->func_id;
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

  if (auto instanceof = vertex.try_as<op_instanceof>()) {
    current_function->class_dep.insert(instanceof->derived_class);
  }

  if (auto catch_op = vertex.try_as<op_catch>()) {
    // since only used functions make it to this point, we may
    // mark class as used even if it was not explicitly instantiated
    // anywhere, so we can compile the try/catch successfully;
    // (if we don't do so, we'll get a crash for unused classes see #68)
    auto klass = catch_op->exception_class;
    kphp_assert(klass);
    klass->mark_as_used();
    current_function->class_dep.insert(klass);
  }

  //NB: There is no user functions in default values of any kind.
  if (auto call = vertex.try_as<op_func_call>()) {
    FunctionPtr other_function = call->func_id;
    // instead of just
    // data.dep.push_back(other_function);
    // we don't add extern function calls, except resumables; why?
    // .dep is used to
    // 1) build call graph — we actually need only user-defined function
    // 2) build resumable call graph — to propagate resumable state and to calc resumable chains
    // 3) build interruptible call graph - calc interruptible chains
    // adding all built-in calls won't affect codegen, it will just overhead call graphs and execution time
    if (!other_function->is_extern() || other_function->is_resumable || other_function->is_imported_from_static_lib()
      || other_function->is_interruptible || other_function->need_generated_stub) {
      data.dep.push_back(other_function);
    }
    calls.push_back(other_function);
    if (other_function->is_extern()) {
      if (other_function->cpp_template_call) {
        const auto *tp = tinf::get_type(call);
        if (auto klass = tp->class_type()) {
          current_function->class_dep.insert(klass);
        }
      }
      // information that comes from functions.txt about the extern functions
      if (other_function->tl_common_h_dep) {
        // functions that use typed rpc should include tl/common.h
        current_function->tl_common_h_dep = true;
      }
      return vertex;
    }

    int cnt_func_params = other_function->param_ids.size();
    if(other_function->has_variadic_param) {
      cnt_func_params--;
    }
    cnt_func_params = std::min(cnt_func_params, static_cast<int>(call->args().size()));
    for (int ii = 0; ii < cnt_func_params; ++ii) {
      auto val = call->args()[ii];
      VarPtr to_var = other_function->param_ids[ii];
      if (to_var->is_reference) { //passed as reference
        while (val->type() == op_index) {
          val = val.as<op_index>()->array();    // from $a['b'] extract $a
        }
        VarPtr from_var = (val->type() == op_var ? val.as<op_var>()->var_id : val.as<op_instance_prop>()->var_id);
        if (from_var->is_in_global_scope()) {
          data.global_ref_edges.emplace_front(from_var, to_var);
        } else if (from_var->is_reference) {
          data.ref_ref_edges.emplace_front(from_var, to_var);
        }
      }
    }
  } else if (auto as_callback_of_builtin = vertex.try_as<op_callback_of_builtin>()) {
    data.dep.push_back(as_callback_of_builtin->func_id);
  } else if (auto var_vertex = vertex.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    // here we add var for later ub check; we store only vars that can potentially lead to ub, see check-ub.cpp
    if ((var->is_global_var() || var->is_class_static_var()) &&   // only globals (not even static inside functions)
        vertex->rl_type == val_l &&                               // only modified in one way or another
        vk::any_of_equal(tinf::get_type(var)->ptype(), tp_array, tp_mixed)) { // only indexable that can be ref-passed
      data.modified_global_vars.push_back(var);
    }
  } else if (auto param = vertex.try_as<op_func_param>()) {
    VarPtr var = param->var()->var_id;
    if (var->is_reference) {
      data.ref_param_vars.emplace_front(var);
    }
  } else if (auto fork = vertex.try_as<op_fork>()) {
    data.forks.emplace_front(fork->func_call()->func_id);
  }

  return vertex;
}

VertexPtr CalcFuncDepPass::on_exit_vertex(VertexPtr vertex) {
  if (vertex->type() == op_func_call) {
    calls.pop_back();
  }

  return vertex;
}


DepData CalcFuncDepPass::get_data() {
  my_unique(&data.dep);
  my_unique(&data.modified_global_vars);
  return std::move(data);
}
