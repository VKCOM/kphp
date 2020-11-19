// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/debug.h"

#include <string>
#include <vector>

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "common/wrappers/to_array.h"

static std::map<Operation, std::string> OPERATION_NAMES;

void fillOperationNames() {
#define FOREACH_OP(x) OPERATION_NAMES[x] = #x;

#include "auto/compiler/vertex/foreach-op.h"
}

std::string debugTokenName(TokenType t) {
  constexpr auto name_pairs = vk::to_array<std::pair<TokenType, const char *>>({
    {tok_empty, "tok_empty"},
    {tok_int_const, "tok_int_const"},
    {tok_float_const, "tok_float_const"},
    {tok_null, "tok_null"},
    {tok_nan, "tok_nan"},
    {tok_inline_html, "tok_inline_html"},
    {tok_str, "tok_str"},
    {tok_str_begin, "tok_str_begin"},
    {tok_str_end, "tok_str_end"},
    {tok_expr_begin, "tok_expr_begin"},
    {tok_expr_end, "tok_expr_end"},
    {tok_var_name, "tok_var_name"},
    {tok_func_name, "tok_func_name"},
    {tok_while, "tok_while"},
    {tok_for, "tok_for"},
    {tok_foreach, "tok_foreach"},
    {tok_if, "tok_if"},
    {tok_else, "tok_else"},
    {tok_elseif, "tok_elseif"},
    {tok_break, "tok_break"},
    {tok_continue, "tok_continue"},
    {tok_echo, "tok_echo"},
    {tok_dbg_echo, "tok_dbg_echo"},
    {tok_var_dump, "tok_var_dump"},
    {tok_function, "tok_function"},
    {tok_fn, "tok_fn"},
    {tok_varg, "tok_varg"},
    {tok_array, "tok_array"},
    {tok_tuple, "tok_tuple"},
    {tok_shape, "tok_shape"},
    {tok_as, "tok_as"},
    {tok_case, "tok_case"},
    {tok_switch, "tok_switch"},
    {tok_class, "tok_class"},
    {tok_interface, "tok_interface"},
    {tok_trait, "tok_trait"},
    {tok_extends, "tok_extends"},
    {tok_implements, "tok_implements"},
    {tok_namespace, "tok_namespace"},
    {tok_use, "tok_use"},
    {tok_const, "tok_const"},
    {tok_default, "tok_default"},
    {tok_do, "tok_do"},
    {tok_eval, "tok_eval"},
    {tok_return, "tok_return"},
    {tok_list, "tok_list"},
    {tok_include, "tok_include"},
    {tok_include_once, "tok_include_once"},
    {tok_require, "tok_require"},
    {tok_require_once, "tok_require_once"},
    {tok_print, "tok_print"},
    {tok_unset, "tok_unset"},
    {tok_var, "tok_var"},
    {tok_global, "tok_global"},
    {tok_static, "tok_static"},
    {tok_final, "tok_final"},
    {tok_abstract, "tok_abstract"},
    {tok_goto, "tok_goto"},
    {tok_isset, "tok_isset"},
    {tok_declare, "tok_declare"},
    {tok_eq1, "tok_eq1"},
    {tok_eq2, "tok_eq2"},
    {tok_eq3, "tok_eq3"},
    {tok_lt, "tok_lt"},
    {tok_gt, "tok_gt"},
    {tok_le, "tok_le"},
    {tok_ge, "tok_ge"},
    {tok_spaceship, "tok_spaceship"},
    {tok_neq2, "tok_neq2"},
    {tok_neq3, "tok_neq3"},
    {tok_neq_lg, "tok_neq_lg"},
    {tok_oppar, "tok_oppar"},
    {tok_clpar, "tok_clpar"},
    {tok_opbrc, "tok_opbrc"},
    {tok_clbrc, "tok_clbrc"},
    {tok_opbrk, "tok_opbrk"},
    {tok_clbrk, "tok_clbrk"},
    {tok_semicolon, "tok_semicolon"},
    {tok_comma, "tok_comma"},
    {tok_dot, "tok_dot"},
    {tok_colon, "tok_colon"},
    {tok_at, "tok_at"},
    {tok_pow, "tok_pow"},
    {tok_inc, "tok_inc"},
    {tok_dec, "tok_dec"},
    {tok_plus, "tok_plus"},
    {tok_minus, "tok_minus"},
    {tok_times, "tok_times"},
    {tok_divide, "tok_divide"},
    {tok_mod, "tok_mod"},
    {tok_and, "tok_and"},
    {tok_or, "tok_or"},
    {tok_xor, "tok_xor"},
    {tok_not, "tok_not"},
    {tok_log_not, "tok_log_not"},
    {tok_question, "tok_question"},
    {tok_null_coalesce, "tok_null_coalesce"},
    {tok_leq, "tok_leq"},
    {tok_shl, "tok_shl"},
    {tok_geq, "tok_geq"},
    {tok_shr, "tok_shr"},
    {tok_neq, "tok_neq"},
    {tok_set_add, "tok_set_add"},
    {tok_set_sub, "tok_set_sub"},
    {tok_set_mul, "tok_set_mul"},
    {tok_set_div, "tok_set_div"},
    {tok_set_mod, "tok_set_mod"},
    {tok_set_pow, "tok_set_pow"},
    {tok_set_and, "tok_set_and"},
    {tok_log_and, "tok_log_and"},
    {tok_log_and_let, "tok_log_and_let"},
    {tok_set_or, "tok_set_or"},
    {tok_log_or, "tok_log_or"},
    {tok_log_or_let, "tok_log_or_let"},
    {tok_set_xor, "tok_set_xor"},
    {tok_log_xor, "tok_log_xor"},
    {tok_log_xor_let, "tok_log_xor_let"},
    {tok_set_shr, "tok_set_shr"},
    {tok_set_shl, "tok_set_shl"},
    {tok_set_dot, "tok_set_dot"},
    {tok_double_arrow, "tok_double_arrow"},
    {tok_double_colon, "tok_double_colon"},
    {tok_arrow, "tok_arrow"},
    {tok_class_c, "tok_class_c"},
    {tok_file_c, "tok_file_c"},
    {tok_func_c, "tok_func_c"},
    {tok_dir_c, "tok_dir_c"},
    {tok_line_c, "tok_line_c"},
    {tok_method_c, "tok_method_c"},
    {tok_namespace_c, "tok_namespace_c"},
    {tok_int, "tok_int"},
    {tok_float, "tok_float"},
    {tok_string, "tok_string"},
    {tok_object, "tok_object"},
    {tok_callable, "tok_callable"},
    {tok_bool, "tok_bool"},
    {tok_void, "tok_void"},
    {tok_mixed, "tok_mixed"},
    {tok_conv_int, "tok_conv_int"},
    {tok_conv_float, "tok_conv_float"},
    {tok_conv_string, "tok_conv_string"},
    {tok_conv_array, "tok_conv_array"},
    {tok_conv_object, "tok_conv_object"},
    {tok_conv_bool, "tok_conv_bool"},
    {tok_conv_var, "tok_conv_var"},
    {tok_conv_uint, "tok_conv_uint"},
    {tok_conv_long, "tok_conv_long"},
    {tok_conv_ulong, "tok_conv_ulong"},
    {tok_false, "tok_false"},
    {tok_true, "tok_true"},
    {tok_define, "tok_define"},
    {tok_defined, "tok_defined"},
    {tok_triple_colon, "tok_triple_colon"},
    {tok_triple_gt, "tok_triple_gt"},
    {tok_triple_lt, "tok_triple_lt"},
    {tok_throw, "tok_throw"},
    {tok_new, "tok_new"},
    {tok_Exception, "tok_Exception"},
    {tok_try, "tok_try"},
    {tok_catch, "tok_catch"},
    {tok_public, "tok_public"},
    {tok_private, "tok_private"},
    {tok_protected, "tok_protected"},
    {tok_phpdoc, "tok_phpdoc"},
    {tok_clone, "tok_clone"},
    {tok_instanceof, "tok_instanceof"},
    {tok_end, "tok_end"},
  });

  // this assert will fail whether TokenType entry is added (or removed);
  // how to fix: add {$newtok, "$newtok"} to the name_pairs
  // hint: insert that entry into the right position
  static_assert(name_pairs.size() - 1 == tok_end,"name_pairs needs to be updated");

#if __cplusplus > 201703
  static_assert(std::is_sorted(name_pairs.begin(), name_pairs.end()), "name_pairs should be sorted");
#else
  static bool sorted = std::is_sorted(name_pairs.begin(), name_pairs.end());
  kphp_assert(sorted);
#endif

  return name_pairs.at(t).second;
}

std::string debugOperationName(Operation o) {
  if (OPERATION_NAMES.empty()) {
    fillOperationNames();
  }

  return OPERATION_NAMES[o];
}


std::string debugVertexMore(VertexPtr v) {
  switch (v->type()) {
    case op_alloc:
      return "new " + v.as<op_alloc>()->allocated_class_name;
    case op_func_call:
      return string(v->extra_type == op_ex_func_call_arrow ? "->" : "") + v->get_string() + "()";
    case op_func_name:
      return v->get_string();
    case op_function:
      return v.as<op_function>()->func_id->name + "()";
    case op_var:
      return "$" + v->get_string();
    case op_instance_prop:
      return "...->" + v->get_string();
    case op_string:
      return v->get_string() == "\n" ? "\"\\n\"" : "\"" + v->get_string() + "\"";
    case op_int_const:
      return v->get_string();
    case op_type_expr_class:
      return v.as<op_type_expr_class>()->class_ptr
             ? v.as<op_type_expr_class>()->class_ptr->name : "class_ptr = null";
    case op_type_expr_type:
      return ptype_name(v->type_help);
    case op_seq:
      return std::to_string(v->size());
    case op_type_expr_arg_ref:
      return "^" + std::to_string(v.as<op_type_expr_arg_ref>()->int_val);
    case op_phpdoc_raw:
      return "/*" + std::string(v.as<op_phpdoc_raw>()->phpdoc_str) + "*/";
    default:
      return "";
  }
}

void debugPrintVertexTree(VertexPtr root, int level) {
  for (int i = 0; i < level; ++i) {
    fmt_print("  ");
  }
  fmt_print("{} {}\n", debugOperationName(root->type()), debugVertexMore(root));

  for (auto i : *root) {
    debugPrintVertexTree(i, level + 1);
  }
}

void debugPrintFunction(FunctionPtr function) {
  fmt_print("--- {}\n", function->name);
  debugPrintVertexTree(function->root, 0);
}


struct GdbVertex {
  Operation type;
  std::string str;

  GdbVertex *ith0;
  GdbVertex *ith1;
  GdbVertex *ith2;
};

GdbVertex *debugVertexToGdb(VertexPtr v) {
  GdbVertex *g = new GdbVertex;
  int size = v ? v->size() : 0;

  g->type = (v ? v->type() : meta_op_base);
  g->str = v ? debugVertexMore(v) : "";
  VertexRange sons{v->begin(), v->end()};
  g->ith0 = size > 0 ? debugVertexToGdb(sons[0]) : nullptr;
  g->ith1 = size > 1 ? debugVertexToGdb(sons[1]) : nullptr;
  g->ith2 = size > 2 ? debugVertexToGdb(sons[2]) : nullptr;

  return g;
}

// "expr*8" in CLion debugger
GdbVertex operator*(VertexPtr v, int n __attribute__((unused))) {
  return *debugVertexToGdb(v);
}

