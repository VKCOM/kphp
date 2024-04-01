// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/clone-strange-const-params.h"

#include "compiler/compiler-core.h"
#include "compiler/data/var-data.h"

VertexPtr CloneStrangeConstParams::on_enter_vertex(VertexPtr root) {
  auto func_call = root.try_as<op_func_call>();
  if (!func_call || func_call->str_val == "make_clone") {
    return root;
  }

  auto params_of_called_func = func_call->func_id->get_params();
  auto passed_params = func_call->args();

  auto clone_param = [](VertexPtr param) {
    auto cloned_param = VertexAdaptor<op_func_call>::create(param).set_location(param);
    cloned_param->str_val = "make_clone";
    cloned_param->func_id = G->get_function(cloned_param->str_val);
    G->stats.cnt_make_clone++;

    return cloned_param;
  };

  if (passed_params.size() > params_of_called_func.size()) {
    auto msg = fmt_format("func: {}, passed_cnt: {}, params_of_func_cnt: {}\n", func_call->func_id->as_human_readable(), passed_params.size(),
                          params_of_called_func.size());
    kphp_assert_msg(false, msg);
  }
  for (size_t i = 0; i < passed_params.size() - func_call->func_id->has_variadic_param; ++i) {
    auto &cur_passed_param = passed_params[i];
    if (func_call->func_id->is_extern()) {
      continue;
    }

    auto cur_func_param = params_of_called_func[i].as<op_func_param>();

    if (!cur_func_param->var()->var_id || !cur_func_param->var()->var_id->is_read_only || cur_func_param->var()->ref_flag) {
      // do not clone elements of arrays to function which gets it by reference
      // foo(&$x) {...} foo($xs[0]);
      continue;
    }
    bool must_be_cloned = vk::any_of_equal(cur_passed_param->type(), op_index, op_instance_prop);
    if (auto passed_var = cur_passed_param.try_as<op_var>()) {
      kphp_assert(passed_var->var_id);
      bool is_this_var = func_call->func_id->modifiers.is_instance() && i == 0;
      must_be_cloned |= (!is_this_var && passed_var->var_id->is_reference) || passed_var->var_id->is_in_global_scope();
    }

    if (must_be_cloned) {
      cur_passed_param = clone_param(cur_passed_param);
    }
  }

  return root;
}
