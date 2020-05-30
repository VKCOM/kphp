#pragma once

#include <map>
#include <stack>
#include <string>

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"

class TransformToSmartInstanceof final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Trasform To Smart Instanceof";
  }

  bool user_recursion(VertexPtr v) override;

  VertexPtr on_enter_vertex(VertexPtr v) override;

private:
  static VertexAdaptor<op_set> generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex);
  static VertexAdaptor<op_instanceof> get_instanceof_from_if(VertexAdaptor<op_if> if_vertex);
  void add_tmp_var_with_instance_cast(VertexAdaptor<op_var> instance_var, VertexPtr name_of_derived, VertexPtr &cmd);

  std::map<std::string, std::stack<std::string>> new_names_of_var;
};
