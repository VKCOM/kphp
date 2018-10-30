#pragma once

#include <string>

#include "compiler/data/class-members.h"
#include "compiler/type-inferer-core.h"

class VarData {
public:
  enum Type {
    var_unknown_t = 0,
    var_local_t,
    var_local_inplace_t,
    var_global_t,
    var_param_t,
    var_const_t,
    var_static_t,
    var_instance_t
  };

  Type type_;
  int id;
  int param_i;
  std::string name;
  tinf::VarNode tinf_node;
  VertexPtr init_val;
  FunctionPtr holder_func;
  FunctionPtr static_id; // id of function if variable is static
  ClassPtr class_id; // id of class if variable is static fields
  vector<VarPtr> *bad_vars;
  bool is_constant;
  bool is_reference;
  bool uninited_flag;
  bool optimize_flag;
  bool tinf_flag;
  bool global_init_flag;
  bool needs_const_iterator_flag;
  bool marked_as_global;
  int dependency_level;

  void set_uninited_flag(bool f);
  bool get_uninited_flag();

  explicit VarData(Type type = var_unknown_t);

  inline Type &type() { return type_; }

  string get_human_readable_name() const;

  inline bool is_global_var() const {
    return type_ == var_global_t && !class_id;
  }

  inline bool is_function_static_var() const {
    return type_ == var_static_t;
  }

  inline bool is_class_static_var() const {
    return type_ == var_global_t && class_id;
  }

  inline bool is_class_instance_var() const {
    return type_ == var_instance_t;
  }

  const ClassMemberStaticField *as_class_static_field() const;
  const ClassMemberInstanceField *as_class_instance_field() const;
};
