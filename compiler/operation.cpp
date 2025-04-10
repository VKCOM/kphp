// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/operation.h"

#include "compiler/vertex.h"

inline void OpInfo::add_binary_op(int priority, TokenType tok, Operation op) {
  tok_to_binary_op[tok] = op;
  set_priority(op, priority);
}

inline void OpInfo::add_unary_op(int priority, TokenType tok, Operation op) {
  tok_to_unary_op[tok] = op;
  set_priority(op, priority);
}

inline void OpInfo::add_op(TokenType tok, Operation op) {
  assert(tok_to_op[tok] == op_err);
  tok_to_op[tok] = op;
}

void OpInfo::init_static() {
  if (was_init_static) {
    return;
  }
  was_init_static = 1;

  static OpProperties default_properties; // TODO was not initialized, static added
  default_properties.cnst = cnst_not_func;
  std::fill(P, P + Operation_size, default_properties);
#define FOREACH_OP(x) VertexAdaptor<x>::init_properties(&P[x]);

#include "auto/compiler/vertex/foreach-op.h"

#undef FOREACH_OP

  memset(tok_to_op, 0, sizeof(tok_to_op));
  memset(tok_to_unary_op, 0, sizeof(tok_to_unary_op));
  memset(tok_to_binary_op, 0, sizeof(tok_to_binary_op));

  int curP = 1;

  op_priority_begin = curP;
  add_binary_op(curP, tok_log_or_let, op_log_or_let);
  curP++;
  add_binary_op(curP, tok_log_xor_let, op_log_xor_let);
  curP++;
  add_binary_op(curP, tok_log_and_let, op_log_and_let);
  curP++;

  add_binary_op(curP, tok_double_arrow, op_double_arrow);
  add_binary_op(curP, tok_eq1, op_set);
  add_binary_op(curP, tok_set_add, op_set_add);
  add_binary_op(curP, tok_set_sub, op_set_sub);
  add_binary_op(curP, tok_set_mul, op_set_mul);
  add_binary_op(curP, tok_set_div, op_set_div);
  add_binary_op(curP, tok_set_mod, op_set_mod);
  add_binary_op(curP, tok_set_pow, op_set_pow);
  add_binary_op(curP, tok_set_and, op_set_and);
  add_binary_op(curP, tok_set_or, op_set_or);
  add_binary_op(curP, tok_set_xor, op_set_xor);
  add_binary_op(curP, tok_set_dot, op_set_dot);
  add_binary_op(curP, tok_set_shr, op_set_shr);
  add_binary_op(curP, tok_set_shl, op_set_shl);
  add_binary_op(curP, tok_set_null_coalesce, op_set_null_coalesce);
  curP++;

  add_binary_op(curP, tok_question, op_ternary);
  ternaryP = curP;
  curP++;

  add_binary_op(curP, tok_null_coalesce, op_null_coalesce);
  curP++;

  add_binary_op(curP, tok_log_or, op_log_or);
  curP++;

  add_binary_op(curP, tok_log_and, op_log_and);
  curP++;

  add_binary_op(curP, tok_or, op_or);
  curP++;

  add_binary_op(curP, tok_xor, op_xor);
  curP++;

  add_binary_op(curP, tok_and, op_and);
  curP++;

  add_binary_op(curP, tok_eq2, op_eq2);
  add_binary_op(curP, tok_eq3, op_eq3);
  add_binary_op(curP, tok_spaceship, op_spaceship);
  curP++;

  add_binary_op(curP, tok_lt, op_lt);
  add_binary_op(curP, tok_le, op_le);
  curP++;

  add_binary_op(curP, tok_shr, op_shr);
  add_binary_op(curP, tok_shl, op_shl);
  curP++;

  add_binary_op(curP, tok_plus, op_add);
  add_binary_op(curP, tok_minus, op_sub);
  add_binary_op(curP, tok_dot, op_concat);
  curP++;

  add_binary_op(curP, tok_times, op_mul);
  add_binary_op(curP, tok_divide, op_div);
  add_binary_op(curP, tok_mod, op_mod);
  curP++;

  add_unary_op(curP, tok_not, op_not);
  add_unary_op(curP, tok_log_not, op_log_not);
  add_unary_op(curP, tok_minus, op_minus);
  add_unary_op(curP, tok_plus, op_plus);
  add_unary_op(curP, tok_and, op_addr);
  ++curP;

  add_binary_op(curP, tok_instanceof, op_instanceof);
  ++curP;

  add_unary_op(curP, tok_conv_int, op_conv_int);
  add_unary_op(curP, tok_conv_float, op_conv_float);
  add_unary_op(curP, tok_conv_string, op_conv_string);
  add_unary_op(curP, tok_conv_array, op_conv_array);
  add_unary_op(curP, tok_conv_object, op_conv_object);
  add_unary_op(curP, tok_conv_bool, op_conv_bool);
  add_unary_op(curP, tok_at, op_noerr);
  ++curP;

  add_binary_op(curP, tok_pow, op_pow);
  ++curP;

  add_unary_op(curP, tok_inc, op_prefix_inc);
  add_unary_op(curP, tok_dec, op_prefix_dec);

  op_priority_end = ++curP;
}

OpProperties OpInfo::P[Operation_size];
int OpInfo::was_init_static = 0;
// TODO: assert that 255 is enough
Operation OpInfo::tok_to_op[255];
Operation OpInfo::tok_to_binary_op[255];
Operation OpInfo::tok_to_unary_op[255];

int OpInfo::op_priority_begin, OpInfo::op_priority_end;
int OpInfo::ternaryP;
