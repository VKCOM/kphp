// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/inferring/node.h"

namespace tinf {

class VarNode : public Node {
public:
  enum {
    e_uninited = -3,
    e_variable = -2,
    e_return_value = -1,
    // 0 and above — it's a function argument
  };

  VarPtr var_;
  int param_i{e_uninited};
  FunctionPtr function_;
  const TypeData *type_restriction{nullptr};

  VarNode() = default;

  void init_as_variable(VarPtr var);
  void init_as_argument(VarPtr var);
  void init_as_return_value(FunctionPtr function);

  void copy_type_from(const TypeData *from) {
    type_ = from;
    recalc_state_ = recalc_st_waiting | recalc_bit_at_least_once;
  }

  void recalc(TypeInferer *inferer) final;

  VarPtr get_var() const {
    return var_;
  }

  std::string get_description() final;
  const Location &get_location() const final;

  bool is_variable() const {
    return param_i == e_variable;
  }

  bool is_return_value_from_function() const {
    return param_i == e_return_value;
  }

  bool is_argument_of_function() const {
    return param_i >= 0;
  }

  void set_type_restriction(const TypeData *r);
};

} // namespace tinf
