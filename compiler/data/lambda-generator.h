#pragma once

#include "compiler/data/lambda-class-data.h"

class LambdaGenerator {
public:
  LambdaGenerator(FunctionPtr function, const Location &location, bool is_static = false);

  LambdaGenerator &add_uses(std::vector<VertexAdaptor<op_func_param>> uses);
  LambdaGenerator &add_invoke_method(const VertexAdaptor<op_function> &function);
  LambdaGenerator &add_invoke_method_which_call_method(FunctionPtr called_method);
  LambdaGenerator &add_invoke_method_which_call_function(FunctionPtr called_function);
  LambdaGenerator &add_constructor_from_uses();

  LambdaPtr generate(FunctionPtr parent_function);
  LambdaPtr generate_and_require(FunctionPtr parent_function, DataStream<FunctionPtr> &os);

private:
  VertexAdaptor<op_func_name> create_name(const std::string &name);
  static LambdaPtr create_class(VertexAdaptor<op_func_name> name);
  VertexAdaptor<op_func_param_list> create_invoke_params(VertexAdaptor<op_function> function);
  VertexAdaptor<op_seq> create_invoke_cmd(VertexAdaptor<op_function> function);

  void add_this_to_captured_variables(VertexPtr &root);

  template<Operation op = op_var>
  VertexAdaptor<op> get_var_of_captured_array_arg();

  void add_uses_for_captured_class_from_array();
  std::vector<VertexAdaptor<op_var>> create_params_for_invoke_which_call_method(FunctionPtr called_method);
  FunctionPtr register_invoke_method(VertexAdaptor<op_function> fun);
  LambdaGenerator &create_invoke_fun_returning_call(FunctionPtr base_fun, VertexAdaptor<op_func_call> &call_function, VertexAdaptor<op_func_param_list> invoke_params);

private:
  const Location created_location;
  VertexAdaptor<op_func_name> lambda_class_name;
  LambdaPtr generated_lambda;
  std::vector<VertexAdaptor<op_func_param>> uses;
};
