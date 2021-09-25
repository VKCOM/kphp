// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "auto/compiler/vertex/vertex-types.h"

#include "compiler/token.h"

enum OperationExtra {
  op_ex_none = 0,
  op_ex_func_call_arrow,
  op_ex_constructor_call,
  op_ex_param_from_use,
  op_ex_param_variadic,
  op_ex_var_const,
  op_ex_var_superglobal,
  op_ex_var_superlocal,
  op_ex_var_superlocal_inplace,
  op_ex_var_this,
  op_ex_internal_func,
  op_ex_safe_version,
};

enum RLValueType {
  val_error = 0,
  val_r,
  val_l,
  val_none
};
enum ConstValueType {
  cnst_error_ = 0,
  cnst_not_val,
  cnst_nonconst_val,
  cnst_const_val
};


struct OpProperties {
  Operation op;
  Operation base_op; // op_add for op_set_add

  int priority;
  opp_fixity_t fixity;

  RLOperationType rl;
  ConstOperationType cnst;
  OperationType type;

  std::string description;
  std::string str;
  std::string op_str;
};

struct OpInfo {
  static int was_init_static;
  //TODO: assert that 255 is enough
  static Operation tok_to_op[255];
  static Operation tok_to_binary_op[255];
  static Operation tok_to_unary_op[255];
  static OpProperties P[Operation_size];

  static int op_priority_begin, op_priority_end;
  static int ternaryP;

  static inline void add_binary_op(int priority, TokenType tok, Operation op);
  static inline void add_unary_op(int priority, TokenType tok, Operation op);
  static inline void add_op(TokenType tok, Operation op);
  static void init_static();

private:
  static inline OpProperties &get_properties(Operation op) {
    return P[op];
  }

  static inline void set_priority(Operation op, int priority) {
    get_properties(op).priority = priority;
  }

public:
  static inline const OpProperties &properties(Operation op) {
    return get_properties(op);
  }

  static inline RLOperationType rl(Operation op) {
    return properties(op).rl;
  }

  static inline ConstOperationType cnst(Operation op) {
    return properties(op).cnst;
  }

  static inline OperationType type(Operation op) {
    return properties(op).type;
  }

  static inline const std::string &desc(Operation op) {
    return properties(op).description;
  }

  static inline const std::string &str(Operation op) {
    return properties(op).str;
  }

  static inline const char *op_str(Operation op) {
    return properties(op).op_str.c_str();
  }

  static inline Operation base_op(Operation op) {
    return properties(op).base_op;
  }

  static inline int priority(Operation op) {
    return properties(op).priority;
  }

  static inline opp_fixity_t fixity(Operation op) {
    return properties(op).fixity;
  }
};

