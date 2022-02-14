// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/common.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/class-member-modifiers.h"
#include "compiler/data/field-modifiers.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/operation.h"
#include "compiler/token.h"
#include "compiler/vertex.h"

class GenTree {

  template<class T>
  class StackPushPop {
    std::vector<T> &stack;
    T &top_element;

  public:
    StackPushPop(std::vector<T> &stack, T &top_element, T elem_to_push) :
      stack(stack),
      top_element(top_element) {
      stack.emplace_back(elem_to_push);
      top_element = elem_to_push;
    }

    ~StackPushPop() {
      stack.pop_back();
      top_element = stack.empty() ? T() : stack.back();
    }
  };

public:
  Location auto_location() const { return Location{this->line_num}; }
  
  VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix);
  static VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix, FunctionPtr cur_function);
  static VertexAdaptor<op_switch> create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition, std::vector<VertexPtr> &&cases);
  static VertexAdaptor<op_match> create_match_vertex(FunctionPtr cur_function, VertexPtr match_condition, std::vector<VertexPtr> &&cases);

  static bool is_superglobal(const string &s);
  static bool is_magic_method_name_allowed(const std::string &name);


  GenTree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

  static VertexAdaptor<op_string> generate_constant_field_class_value(ClassPtr klass);

  bool test_expect(TokenType tp) { return cur->type() == tp; }

  void next_cur();
  int open_parent();
  void skip_phpdoc_tokens();

  static VertexAdaptor<op_seq> embrace(VertexPtr v);

  template<PrimitiveType ToT>
  static VertexPtr conv_to(VertexPtr x);

  static VertexPtr get_actual_value(VertexPtr v);
  static const std::string *get_constexpr_string(VertexPtr v);
  static VertexPtr get_call_arg_ref(int arg_num, VertexPtr v_func_call);
  static VertexPtr get_call_arg_for_param(VertexAdaptor<op_func_call> call, VertexAdaptor<op_func_param> param, int param_i);

  static void func_force_return(VertexAdaptor<op_function> func, VertexPtr val = {});
  VertexAdaptor<op_ternary> create_ternary_op_vertex(VertexPtr condition, VertexPtr true_expr, VertexPtr false_expr);

  static auto create_int_const(int32_t number) {
    auto int_v = VertexAdaptor<op_int_const>::create();
    int_v->str_val = std::to_string(number);
    return int_v;
  }

  static auto create_string_const(const std::string &str) {
    auto string_vertex = VertexAdaptor<op_string>::create();
    string_vertex->str_val = str;
    return string_vertex;
  }

  VertexAdaptor<op_func_param> get_func_param();
  VertexAdaptor<op_var> get_var_name();
  VertexAdaptor<op_var> get_var_name_ref();
  VertexPtr get_expr_top(bool was_arrow);
  VertexPtr get_postfix_expression(VertexPtr res, bool parenthesized);
  VertexPtr get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary);
  VertexPtr get_binary_op(int op_priority_cur, bool till_ternary);
  VertexPtr get_expression_impl(bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement(vk::string_view phpdoc_str = vk::string_view{});
  VertexAdaptor<op_catch> get_catch();
  void get_instance_var_list(vk::string_view phpdoc_str, FieldModifiers modifiers, const TypeHint *type_hint);
  void get_traits_uses();
  void get_use();
  void get_seq(std::vector<VertexPtr> &seq_next);
  VertexAdaptor<op_seq> get_seq();

  void parse_declare_at_top_of_file();
  void parse_namespace_and_uses_at_top_of_file();
  bool check_seq_end();
  bool check_statement_end();
  void run();

  template<Operation EmptyOp, class FuncT, class ResultType = typename vk::function_traits<FuncT>::ResultType>
  bool gen_list(std::vector<ResultType> *res, FuncT f, TokenType delim);
  template<Operation Op>
  VertexAdaptor<Op> get_conv();
  VertexAdaptor<op_require> get_require(bool once);
  template<Operation Op, Operation EmptyOp>
  VertexAdaptor<Op> get_func_call();
  VertexAdaptor<op_array> get_short_array();
  VertexAdaptor<op_string> get_string();
  VertexAdaptor<op_string_build> get_string_build();
  VertexPtr get_def_value();
  template<PrimitiveType ToT>
  static VertexAdaptor<meta_op_unary> conv_to_lval(VertexPtr x);
  template<Operation Op, Operation EmptyOp, class FuncT, class ResultType = typename vk::function_traits<FuncT>::ResultType>
  VertexAdaptor<op_seq> get_multi_call(FuncT &&f, bool parenthesis = false);
  VertexAdaptor<op_return> get_return();
  template<Operation Op>
  VertexAdaptor<Op> get_break_or_continue();
  VertexAdaptor<op_foreach> get_foreach();
  VertexAdaptor<op_while> get_while();
  VertexAdaptor<op_if> get_if();
  VertexAdaptor<op_for> get_for();
  VertexAdaptor<op_do> get_do();

  VertexAdaptor<op_switch> get_switch();
  VertexAdaptor<op_match_default> get_match_default();
  VertexPtr get_match_case();
  VertexAdaptor<op_match> get_match();

  VertexAdaptor<op_shape> get_shape();
  VertexPtr get_phpdoc_inside_function();
  bool parse_cur_function_uses();
  static bool test_if_uses_and_arguments_intersect(const std::forward_list<VertexAdaptor<op_var>> &uses_list, const VertexRange &params);
  VertexAdaptor<op_lambda> get_lambda_function(vk::string_view phpdoc_str, FunctionModifiers modifiers);
  VertexAdaptor<op_function> get_function(bool is_lambda, vk::string_view phpdoc_str, FunctionModifiers modifiers);

  ClassMemberModifiers parse_class_member_modifier_mask();
  VertexPtr get_class_member(vk::string_view phpdoc_str);

  std::string get_identifier();

  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(const std::string &allocated_class_name, std::vector<VertexPtr> args, const Location &location);
  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(ClassPtr allocated_class, std::vector<VertexPtr> args, const Location &locaction);
  static VertexAdaptor<op_var> auto_capture_this_in_lambda(FunctionPtr f_lambda);

  VertexPtr get_class(vk::string_view phpdoc_str, ClassType class_type);
  void parse_extends_implements();

  static VertexPtr process_arrow(VertexPtr lhs, VertexPtr rhs);

private:
  const TypeHint *get_typehint();

  static void check_and_remove_num_separators(std::string &s);
  VertexPtr get_op_num_const();

  VertexAdaptor<op_func_param_list> parse_cur_function_param_list();

  VertexAdaptor<op_empty> get_static_field_list(vk::string_view phpdoc_str, FieldModifiers modifiers, const TypeHint *type_hint);
  VertexAdaptor<op_var> get_function_use_var_name_ref();
  VertexPtr get_foreach_value();
  std::pair<VertexAdaptor<op_foreach_param>, VertexPtr> get_foreach_param();

  VertexPtr get_const_after_explicit_access_modifier(AccessModifiers access);
  VertexPtr get_const(AccessModifiers access);


  int line_num{0};
  const vector<Token> tokens;
  DataStream<FunctionPtr> &parsed_os;
  bool is_top_of_the_function_{false};
  decltype(tokens)::const_iterator cur, end;
  vector<ClassPtr> class_stack;
  ClassPtr cur_class;               // = class_stack.back()
  vector<FunctionPtr> functions_stack;
  FunctionPtr cur_function;         // = functions_stack.back()
  SrcFilePtr processing_file;
};

template<PrimitiveType ToT>
VertexAdaptor<meta_op_unary> GenTree::conv_to_lval(VertexPtr x) {
  switch (ToT) {
    case tp_array : return VertexAdaptor<op_conv_array_l>::create(x).set_location(x);
    case tp_int   : return VertexAdaptor<op_conv_int_l>::create(x).set_location(x);
    case tp_string: return VertexAdaptor<op_conv_string_l>::create(x).set_location(x);
  }
  return {};
}

template<PrimitiveType ToT>
VertexPtr GenTree::conv_to(VertexPtr x) {
  switch (ToT) {
    case tp_int:
      return VertexAdaptor<op_conv_int>::create(x).set_location(x);

    case tp_bool:
      return VertexAdaptor<op_conv_bool>::create(x).set_location(x);

    case tp_string:
      return VertexAdaptor<op_conv_string>::create(x).set_location(x);

    case tp_float:
      return VertexAdaptor<op_conv_float>::create(x).set_location(x);

    case tp_array:
      return VertexAdaptor<op_conv_array>::create(x).set_location(x);

    case tp_regexp:
      return VertexAdaptor<op_conv_regexp>::create(x).set_location(x);

    case tp_mixed:
      return VertexAdaptor<op_conv_mixed>::create(x).set_location(x);

    default:
      return x;
  }
}

static inline bool is_const_int(VertexPtr root) {
  switch (root->type()) {
    case op_int_const:
      return true;
    case op_minus:
    case op_plus:
    case op_not:
      return is_const_int(root.as<meta_op_unary>()->expr());
    case op_add:
    case op_mul:
    case op_sub:
    case op_div:
    case op_and:
    case op_or:
    case op_xor:
    case op_shl:
    case op_shr:
    case op_mod:
    case op_pow:
      return is_const_int(root.as<meta_op_binary>()->lhs()) && is_const_int(root.as<meta_op_binary>()->rhs());
    default:
      break;
  }
  return false;
}

inline bool is_positive_constexpr_int(VertexPtr v) {
  auto actual_value = GenTree::get_actual_value(v).try_as<op_int_const>();
  return actual_value && parse_int_from_string(actual_value) >= 0;
}

inline bool is_constructor_call(VertexAdaptor<op_func_call> call) {
  return !call->args().empty() && call->str_val == ClassData::NAME_OF_CONSTRUCT;
}
