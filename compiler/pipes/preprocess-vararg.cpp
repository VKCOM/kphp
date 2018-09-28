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
      auto index = VertexAdaptor<op_index>::create(arr, call->ith(0));
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
    VertexPtr old_params = root.as<op_function>()->params();
    vector<VertexPtr> params_varg;
    VertexPtr va_list_var = create_va_list_var(root->location);
    auto va_list_param = VertexAdaptor<op_func_param>::create(va_list_var);
    params_varg.push_back(va_list_param);
    auto params_new = VertexAdaptor<op_func_param_list>::create(params_varg);

    root.as<op_function>()->params() = params_new;

    vector<VertexPtr> params_init;
    int ii = 0;
    for (auto i : *old_params) {
      VertexAdaptor<op_func_param> arg = i;
      kphp_error (!arg->ref_flag, "functions with reference arguments are not supported in vararg");
      VertexPtr var = arg->var();
      VertexPtr def;
      if (arg->has_default_value()) {
        def = arg->default_value();
      } else {
        auto null = VertexAdaptor<op_null>::create();
        def = null;
      }

      auto id0 = VertexAdaptor<op_int_const>::create();
      id0->str_val = int_to_str(ii);
      auto isset_value = VertexAdaptor<op_index>::create(create_va_list_var(root->location), id0);
      auto isset = VertexAdaptor<op_isset>::create(isset_value);

      auto id1 = VertexAdaptor<op_int_const>::create();
      id1->str_val = int_to_str(ii);
      auto result_value = VertexAdaptor<op_index>::create(create_va_list_var(root->location), id1);


      auto expr = VertexAdaptor<op_ternary>::create(isset, result_value, def);
      auto set = VertexAdaptor<op_set>::create(var, expr);
      params_init.push_back(set);
      ii++;
    }

    if (!params_init.empty()) {
      VertexPtr seq = root.as<op_function>()->cmd();
      kphp_assert(seq->type() == op_seq);
      for (auto i : *seq) {
        params_init.push_back(i);
      }
      auto new_seq = VertexAdaptor<op_seq>::create(params_init);
      root.as<op_function>()->cmd() = new_seq;
    }
  }
  return root;
}
bool PreprocessVarargPass::check_function(FunctionPtr function) {
  return function->varg_flag && default_check_function(function);
}
