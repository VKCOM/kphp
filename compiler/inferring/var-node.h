#pragma once

#include "compiler/inferring/node.h"

namespace tinf {

class VarNode : public Node {
public:
  enum {
    e_variable = -2,
    e_return_value = -1
  };

  VarPtr var_;
  int param_i;
  FunctionPtr function_;

  explicit VarNode(VarPtr var = VarPtr()) :
    var_(var),
    param_i(e_variable) {}

  void copy_type_from(const TypeData *from) {
    type_ = from;
    recalc_cnt_ = 1;
  }

  void recalc(TypeInferer *inferer);

  VarPtr get_var() const {
    return var_;
  }

  string get_description();
  string get_var_name();
  string get_function_name();
  string get_var_as_argument_name();

  bool is_variable() const {
    return param_i == e_variable;
  }

  bool is_return_value_from_function() const {
    return param_i == e_return_value;
  }

  bool is_argument_of_function() const {
    return !is_variable() && !is_return_value_from_function();
  }
};

} // namespace tinf
