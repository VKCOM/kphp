// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string>

#include "compiler/data/class-members.h"
#include "compiler/debug.h"
#include "compiler/inferring/var-node.h"

class VarData {
  DEBUG_STRING_METHOD { return get_human_readable_name(); }
  
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

  Type type_ = var_unknown_t;
  int id = -1;
  int param_i = -1;
  std::string name;
  tinf::VarNode tinf_node;
  VertexPtr init_val;
  FunctionPtr holder_func;
  ClassPtr class_id;
  std::unordered_set<VarPtr> *bad_vars = nullptr;
  bool is_reference = false;
  bool uninited_flag = false;
  bool optimize_flag = false;
  bool tinf_flag = false;
  bool needs_const_iterator_flag = false;
  bool marked_as_global = false;
  bool marked_as_const = false;
  bool is_read_only = true;
  bool is_foreach_reference = false;
  int dependency_level = 0;

  void set_uninited_flag(bool f);
  bool get_uninited_flag();

  explicit VarData(Type type);

  inline Type &type() { return type_; }

  std::string get_human_readable_name() const;

  inline bool is_global_var() const {
    return type_ == var_global_t && !class_id;
  }

  inline bool is_in_global_scope() const {
    return type_ == var_global_t || type_ == var_static_t;
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

  inline bool is_constant() const {
    return type_ == var_const_t;
  }

  inline bool is_builtin_global() const {
    return type_ == var_global_t && does_name_eq_any_builtin_global(name);
  }

  const ClassMemberStaticField *as_class_static_field() const;
  const ClassMemberInstanceField *as_class_instance_field() const;

  static bool does_name_eq_any_builtin_global(const std::string &name);
};
