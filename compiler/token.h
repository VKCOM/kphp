// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/wrappers/string_view.h"

enum TokenType {
  tok_empty,
  tok_int_const,
  tok_int_const_sep, // 1234_5678_9123
  tok_float_const,
  tok_float_const_sep, // 10_999.1000_9999
  tok_null,
  tok_nan,
  tok_inf,
  tok_inline_html,
  tok_str,
  tok_str_begin,
  tok_str_end,
  tok_str_skip_indent,
  tok_expr_begin,
  tok_expr_end,
  tok_var_name,
  tok_func_name,
  tok_while,
  tok_for,
  tok_foreach,
  tok_if,
  tok_else,
  tok_elseif,
  tok_break,
  tok_continue,
  tok_echo,
  tok_dbg_echo,
  tok_var_dump,
  tok_function,
  tok_fn,
  tok_varg,
  tok_array,
  tok_tuple,
  tok_shape,
  tok_as,
  tok_case,
  tok_switch,
  tok_class,
  tok_interface,
  tok_trait,
  tok_extends,
  tok_implements,
  tok_namespace,
  tok_use,
  tok_const,
  tok_default,
  tok_do,
  tok_eval,
  tok_return,
  tok_list,
  tok_include,
  tok_include_once,
  tok_require,
  tok_require_once,
  tok_print,
  tok_unset,
  tok_var,
  tok_global,
  tok_static,
  tok_final,
  tok_abstract,
  tok_goto,
  tok_isset,
  tok_declare,
  tok_eq1,
  tok_eq2,
  tok_eq3,
  tok_lt,
  tok_gt,
  tok_le,
  tok_ge,
  tok_spaceship,
  tok_neq2,
  tok_neq3,
  tok_neq_lg,
  tok_oppar,
  tok_clpar,
  tok_opbrc,
  tok_clbrc,
  tok_opbrk,
  tok_clbrk,
  tok_semicolon,
  tok_comma,
  tok_dot,
  tok_colon,

  tok_at,

  tok_pow,
  tok_inc,
  tok_dec,
  tok_plus,
  tok_minus,
  tok_times,
  tok_divide,
  tok_mod,
  tok_and,
  tok_or,
  tok_xor,
  tok_not,
  tok_log_not,
  tok_question,
  tok_null_coalesce,

  tok_leq,
  tok_shl,
  tok_geq,
  tok_shr,
  tok_neq,
  tok_set_add,
  tok_set_sub,
  tok_set_mul,
  tok_set_div,
  tok_set_mod,
  tok_set_pow,
  tok_set_and,
  tok_log_and,
  tok_log_and_let,
  tok_set_or,
  tok_log_or,
  tok_log_or_let,
  tok_set_xor,
  tok_log_xor,
  tok_log_xor_let,
  tok_set_shr,
  tok_set_shl,
  tok_set_dot,
  tok_set_null_coalesce,
  tok_double_arrow,
  tok_double_colon,
  tok_arrow,

  tok_class_c,
  tok_file_c,
  tok_file_relative_c,
  tok_func_c,
  tok_dir_c,
  tok_line_c,
  tok_method_c,
  tok_namespace_c,

  tok_int,
  tok_float,
  tok_double,
  tok_string,
  tok_object,
  tok_callable,
  tok_bool,
  tok_boolean,
  tok_void,
  tok_mixed,

  tok_conv_int,
  tok_conv_float,
  tok_conv_string,
  tok_conv_array,
  tok_conv_object,
  tok_conv_bool,

  tok_false,
  tok_true,

  tok_define,
  tok_defined,

  tok_triple_colon,

  tok_throw,
  tok_new,

  tok_try,
  tok_catch,

  tok_public,
  tok_private,
  tok_protected,

  tok_phpdoc,
  tok_commentTs,

  tok_clone,
  tok_instanceof,

  tok_end
};

class Token {
public:
  TokenType type_;
  int line_num = -1;
  vk::string_view str_val;
  vk::string_view debug_str;

  explicit Token(TokenType type) :
    type_(type) {
  }

  Token(TokenType type_, vk::string_view s) :
    type_(type_),
    str_val(s) {
  }

  Token(TokenType type_, const char *s, const char *t) :
    type_(type_),
    str_val(s, t) {
  }

  // use 'cur->debugPrint()' anywhere in gentree while development
  // (in release it's not used and is not linked to a binary)
  void debugPrint() const {
    std::string debugTokenName(TokenType t);  // implemented in debug.cpp
    printf("%s '%s'\n", debugTokenName(type_).c_str(), static_cast<std::string>(debug_str.empty() ? str_val : debug_str).c_str());
  }

  inline TokenType &type() { return type_; }
  inline const TokenType &type() const { return type_; }

  std::string to_str() const {
    return static_cast<std::string>(debug_str);
  }
};
