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
#include "compiler/data/lambda-generator.h"
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
  Location auto_location() const;
  VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix);
  static VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix, FunctionPtr cur_function);
  static VertexAdaptor<op_switch> create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition, std::vector<VertexPtr> &&cases);

  static bool is_superglobal(const string &s);
  static bool is_magic_method_name_allowed(const std::string &name);


  GenTree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

  static VertexAdaptor<op_string> generate_constant_field_class_value(ClassPtr klass);

  bool test_expect(TokenType tp);

  void next_cur();
  int open_parent();
  void skip_phpdoc_tokens();

  static VertexAdaptor<op_seq> embrace(VertexPtr v);

  static VertexPtr conv_to_cast_param(VertexPtr x, const TypeHint *type_hint, bool ref_flag = 0);
  template<PrimitiveType ToT>
  static VertexPtr conv_to(VertexPtr x);
  static VertexPtr get_actual_value(VertexPtr v);
  static const std::string *get_constexpr_string(VertexPtr v);
  static VertexPtr get_call_arg_ref(int arg_num, VertexPtr expr);

  static void func_force_return(VertexAdaptor<op_function> func, VertexPtr val = {});
  VertexAdaptor<op_ternary> create_ternary_op_vertex(VertexPtr condition, VertexPtr true_expr, VertexPtr false_expr);

  static auto create_int_const(int32_t number) {
    auto int_v = VertexAdaptor<op_int_const>::create();
    int_v->str_val = std::to_string(number);
    return int_v;
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
  VertexAdaptor<Op> get_func_call(std::string name);
  template<Operation Op, Operation EmptyOp>
  VertexAdaptor<Op> get_func_call();
  VertexAdaptor<op_func_call> get_type_and_args_of_new_expression();
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
  VertexAdaptor<op_shape> get_shape();
  VertexPtr get_phpdoc_inside_function();
  bool parse_function_uses(std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda);
  static bool check_uses_and_args_are_not_intersecting(const std::vector<VertexAdaptor<op_func_param>> &uses, const VertexRange &params);
  VertexAdaptor<op_func_call> get_anonymous_function(TokenType tok = tok_function, bool is_static = false);
  VertexAdaptor<op_function> get_function(TokenType tok, vk::string_view phpdoc_str, FunctionModifiers modifiers, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda = nullptr);

  ClassMemberModifiers parse_class_member_modifier_mask();
  VertexPtr get_class_member(vk::string_view phpdoc_str);

  std::string get_identifier();

  static LambdaGenerator generate_anonymous_class(VertexAdaptor<op_function> function,
                                                  FunctionPtr parent_function,
                                                  bool is_static,
                                                  std::vector<VertexAdaptor<op_func_param>> &&uses_of_lambda,
                                                  FunctionPtr already_created_function = FunctionPtr{});

  static VertexAdaptor<op_func_call> generate_call_on_instance_var(VertexPtr instance_var, FunctionPtr function, vk::string_view function_name);
  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(std::string allocated_class_name, std::vector<VertexPtr> args);
  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(ClassPtr allocated_class, std::vector<VertexPtr> args);

  VertexAdaptor<op_func_call> get_class(vk::string_view phpdoc_str, ClassType class_type, bool is_anonymous = false);
  void parse_extends_implements();

  static VertexPtr process_arrow(VertexPtr lhs, VertexPtr rhs);

  static VertexAdaptor<op_func_call> generate_critical_error(std::string msg);

private:
  const TypeHint *get_typehint();

  VertexAdaptor<op_func_param_list> parse_cur_function_param_list();

  VertexAdaptor<op_empty> get_static_field_list(vk::string_view phpdoc_str, FieldModifiers modifiers, const TypeHint *type_hint);
  VertexAdaptor<op_var> get_function_use_var_name_ref();
  VertexPtr get_foreach_value();
  std::pair<VertexAdaptor<op_foreach_param>, VertexPtr> get_foreach_param();

  void require_lambdas() {
    for (auto &generator : lambda_generators) {
      generator.require(parsed_os);
    }
    lambda_generators.clear();
  }

  VertexPtr get_const_after_explicit_access_modifier(AccessModifiers access);
  VertexPtr get_const(AccessModifiers access);

public:
  int line_num{0};

private:
  const vector<Token> tokens;
  DataStream<FunctionPtr> &parsed_os;
  bool is_top_of_the_function_{false};
  decltype(tokens)::const_iterator cur, end;
  vector<ClassPtr> class_stack;
  ClassPtr cur_class;               // = class_stack.back()
  vector<FunctionPtr> functions_stack;
  FunctionPtr cur_function;         // = functions_stack.back()
  SrcFilePtr processing_file;
  std::vector<LambdaGenerator> lambda_generators;
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
