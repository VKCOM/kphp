// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/vertex.h"

// This class contains helper methods that operate on various vertices.
//
// GenTree is mostly for parsing (tokens -> vertex tree),
// VertexUtil is for the operations that are useful in most other parts of the compiler
// and are not directly connected to parsing.
class VertexUtil {
public:
  static VertexPtr get_actual_value(VertexPtr v);
  static VertexPtr unwrap_inlined_define(VertexPtr v);
  static const std::string *get_constexpr_string(VertexPtr v);
  static VertexPtr get_call_arg_ref(int arg_num, VertexPtr v_func_call);
  static VertexAdaptor<op_func_call> add_call_arg(VertexPtr to_add, VertexAdaptor<op_func_call> call, bool prepend);

  static VertexPtr create_conv_to(PrimitiveType targetType, VertexPtr x);
  static VertexAdaptor<meta_op_unary> create_conv_to_lval(PrimitiveType targetType, VertexPtr x);
  static VertexPtr create_int_const(int64_t number);
  static VertexAdaptor<op_string> create_string_const(const std::string &s);
  static VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix, FunctionPtr cur_function);
  static VertexAdaptor<op_switch> create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition, std::vector<VertexPtr> &&cases);

  static VertexPtr unwrap_array_value(VertexPtr v);
  static VertexPtr unwrap_string_value(VertexPtr v);
  static VertexPtr unwrap_int_value(VertexPtr v);
  static VertexPtr unwrap_float_value(VertexPtr v);
  static VertexPtr unwrap_bool_value(VertexPtr v);

  static VertexAdaptor<op_seq> embrace(VertexPtr v);

  static void func_force_return(VertexAdaptor<op_function> func, VertexPtr val = {});

  static bool is_superglobal(const std::string &s);
  static bool is_positive_constexpr_int(VertexPtr v);
  static bool is_const_int(VertexPtr root);
};
