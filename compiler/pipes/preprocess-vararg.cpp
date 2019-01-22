#include "compiler/pipes/preprocess-vararg.h"

VertexPtr PreprocessVarargPass::create_va_list_var(Location loc) {
  auto result = VertexAdaptor<op_var>::create();
  result->str_val = "$VA_LIST";
  set_location(result, loc);
  return result;
}
VertexPtr PreprocessVarargPass::on_enter_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_func_call) {
    VertexPtr call = root.as<op_func_call>();
    string name = call->get_string();
    if (name == "func_get_args") {
      kphp_error(call->size() == 0, "Strange func_get_args with arguments");
      return create_va_list_var(call->location);
    } else if (name == "func_get_arg") {
      kphp_error(call->size() == 1, "Strange func_num_arg not one argument");
      VertexPtr arr = create_va_list_var(call->location);
      auto index = VertexAdaptor<op_index>::create(arr, call.as<op_func_call>()->args()[0]);
      return index;
    } else if (name == "func_num_args") {
      kphp_error(call->size() == 0, "Strange func_num_args with arguments");
      VertexPtr arr = create_va_list_var(call->location);
      auto count_call = VertexAdaptor<op_func_call>::create(arr);
      count_call->str_val = "count";
      set_location(count_call, call->location);
      return count_call;
    }
    return root;
  }
  if (root->type() == op_function) {
    // у инстанс-функций есть неявный $this, который сейчас old_params[0]
    // тогда f конвертируем не в f($VA_LIST), а f($this, $VA_LIST); обычные функции — f($VA_LIST)
    vector<VertexPtr> old_params = root.as<op_function>()->params()->get_next();
    vector<VertexPtr> new_params;

    int rest_start_pos = current_function->has_implicit_this_arg() ? 1 : 0;
    new_params.insert(new_params.begin(), old_params.begin(), old_params.begin() + rest_start_pos);

    VertexPtr va_list_var = create_va_list_var(root->location);
    new_params.emplace_back(VertexAdaptor<op_func_param>::create(va_list_var));

    root.as<op_function>()->params() = VertexAdaptor<op_func_param_list>::create(new_params);

    // если функция объявлена f($first) и использует func_get_args(), то она превратилась в f($VA_LIST)
    // и тут делаем { $first = isset($VA_LIST[0]) ? $VA_LIST[0] : null; } в начало тела f
    vector<VertexPtr> params_init;
    for (int i = rest_start_pos; i < old_params.size(); ++i) {
      VertexAdaptor<op_func_param> arg = old_params[i];
      kphp_error (!arg->ref_flag, "functions with reference arguments are not supported in vararg");
      VertexPtr var = arg->var();
      VertexPtr def;
      if (arg->has_default_value() && arg->default_value()) {
        def = arg->default_value();
      } else {
        def = VertexAdaptor<op_null>::create();
      }

      auto id0 = VertexAdaptor<op_int_const>::create();
      id0->str_val = int_to_str(i - rest_start_pos);
      auto isset_value = VertexAdaptor<op_index>::create(create_va_list_var(root->location), id0);
      auto isset = VertexAdaptor<op_isset>::create(isset_value);

      auto id1 = VertexAdaptor<op_int_const>::create();
      id1->str_val = int_to_str(i - rest_start_pos);
      auto result_value = VertexAdaptor<op_index>::create(create_va_list_var(root->location), id1);

      auto expr = VertexAdaptor<op_ternary>::create(isset, result_value, def);
      auto set = VertexAdaptor<op_set>::create(var, expr);
      params_init.push_back(set);
    }

    if (!params_init.empty()) {
      VertexPtr seq = root.as<op_function>()->cmd();
      params_init.insert(params_init.end(), seq->begin(), seq->end());
      root.as<op_function>()->cmd() = VertexAdaptor<op_seq>::create(params_init);
    }
  }
  return root;
}
bool PreprocessVarargPass::check_function(FunctionPtr function) {
  return function->is_vararg && default_check_function(function);
}
