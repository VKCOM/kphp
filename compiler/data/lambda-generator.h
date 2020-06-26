#pragma once

#include "compiler/data/lambda-class-data.h"

class LambdaGenerator {
public:
  LambdaGenerator(FunctionPtr function, const Location &location, bool is_static = false);

  LambdaGenerator &add_uses(std::vector<VertexAdaptor<op_func_param>> uses);
  LambdaGenerator &add_invoke_method(const VertexAdaptor<op_function> &function, FunctionPtr already_created_function = {});
  LambdaGenerator &add_invoke_method_which_call_method(FunctionPtr called_method);
  LambdaGenerator &add_invoke_method_which_call_function(FunctionPtr called_function);
  LambdaGenerator &add_constructor_from_uses();

  LambdaGenerator &generate(FunctionPtr parent_function);
  LambdaGenerator &require(DataStream<FunctionPtr> &os);
  LambdaPtr generate_and_require(FunctionPtr parent_function, DataStream<FunctionPtr> &os);

  LambdaPtr get_generated_lambda() const {
    return generated_lambda;
  }

  static void make_invoke_method(ClassPtr klass, std::string fun_name, VertexAdaptor<op_function> fun);

private:
  static LambdaPtr create_class(const std::string &name);
  VertexAdaptor<op_func_param_list> create_invoke_params(VertexAdaptor<op_function> function);
  VertexAdaptor<op_seq> create_invoke_cmd(VertexAdaptor<op_function> function);
  VertexAdaptor<op_seq> create_seq_saving_captured_vars_to_fields(const std::vector<VertexAdaptor<op_func_param>> &params);

  void add_this_to_captured_variables(VertexPtr &root);

  template<Operation op = op_var>
  VertexAdaptor<op> get_var_of_captured_array_arg();

  void add_uses_for_captured_class_from_array();
  std::vector<VertexAdaptor<op_var>> create_params_for_invoke_which_call_method(FunctionPtr called_method);
  void register_invoke_method(std::string fun_name, VertexAdaptor<op_function> fun);
  LambdaGenerator &create_invoke_fun_returning_call(FunctionPtr base_fun, VertexAdaptor<op_func_call> &call_function, VertexAdaptor<op_func_param_list> invoke_params);

private:
  const Location created_location;
  LambdaPtr generated_lambda;
  std::vector<VertexAdaptor<op_func_param>> uses;
};
