// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/class-member-modifiers.h"
#include "compiler/data/field-modifiers.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/operation.h"
#include "compiler/token.h"
#include "compiler/vertex.h"

class GenTree {

  enum class EnumType {
    Empty,
    Pure,
    BackedString,
    BackedInt,
  };

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

  static bool is_magic_method_name_allowed(const std::string &name);


  GenTree(std::vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

  bool test_expect(TokenType tp) { return cur->type() == tp; }

  void next_cur();
  int open_parent();
  void skip_phpdoc_tokens();

  static VertexPtr get_call_arg_for_param(VertexAdaptor<op_func_call> call, VertexAdaptor<op_func_param> param, int param_i);

  VertexAdaptor<op_ternary> create_ternary_op_vertex(VertexPtr condition, VertexPtr true_expr, VertexPtr false_expr);

  VertexAdaptor<op_func_param> get_func_param();
  VertexAdaptor<op_var> get_var_name();
  VertexAdaptor<op_var> get_var_name_ref();
  VertexPtr get_expr_top(bool was_arrow, const PhpDocComment *phpdoc = nullptr);
  VertexPtr get_postfix_expression(VertexPtr res, bool parenthesized);
  VertexPtr get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary);
  VertexPtr get_binary_op(int op_priority_cur, bool till_ternary);
  VertexPtr get_expression_impl(bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement(const PhpDocComment *phpdoc = nullptr);
  VertexAdaptor<op_catch> get_catch();
  void get_instance_var_list(const PhpDocComment *phpdoc, FieldModifiers modifiers, const TypeHint *type_hint);
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
  VertexPtr get_by_name_construct();
  VertexPtr get_member_by_name_after_var(VertexAdaptor<op_var> v_before);
  VertexPtr get_phpdoc_inside_function();
  bool parse_cur_function_uses();
  static bool test_if_uses_and_arguments_intersect(const std::forward_list<VertexAdaptor<op_var>> &uses_list, const VertexRange &params);
  VertexAdaptor<op_lambda> get_lambda_function(const PhpDocComment *phpdoc, FunctionModifiers modifiers);
  VertexAdaptor<op_function> get_function(bool is_lambda, const PhpDocComment *phpdoc, FunctionModifiers modifiers);

  ClassMemberModifiers parse_class_member_modifier_mask();
  VertexPtr get_class_member(const PhpDocComment *phpdoc);

  std::string get_identifier();

  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(const std::string &allocated_class_name, std::vector<VertexPtr> args, const Location &location);
  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(ClassPtr allocated_class, std::vector<VertexPtr> args, const Location &locaction);
  static VertexAdaptor<op_var> auto_capture_this_in_lambda(FunctionPtr f_lambda);

  VertexPtr get_class(const PhpDocComment *phpdoc, ClassType class_type);
  void parse_extends();
  void parse_implements();
  void parse_extends_implements();

  VertexPtr get_enum(const PhpDocComment *phpdoc);
  VertexAdaptor<op_case> get_enum_case();
  EnumType get_enum_type();
  std::vector<std::string> get_enum_body_and_cases(EnumType enum_type);
  void generate_enum_fields(EnumType enum_type);
  void generate_enum_construct(EnumType enum_type);
  void generate_pure_enum_methods(const std::vector<std::string> &cases);
  void generate_backed_enum_methods();


  static VertexPtr process_arrow(VertexPtr lhs, VertexPtr rhs);

private:
  const TypeHint *get_typehint();
  GenericsInstantiationPhpComment *parse_php_commentTs(vk::string_view str_commentTs);

  static void check_and_remove_num_separators(std::string &s);
  VertexPtr get_op_num_const();

  VertexAdaptor<op_func_param_list> parse_cur_function_param_list();

  VertexAdaptor<op_empty> get_static_field_list(const PhpDocComment *phpdoc, FieldModifiers modifiers, const TypeHint *type_hint);
  VertexAdaptor<op_var> get_function_use_var_name_ref();
  VertexPtr get_foreach_value();
  std::pair<VertexAdaptor<op_foreach_param>, VertexPtr> get_foreach_param();

  VertexPtr get_const_after_explicit_access_modifier(AccessModifiers access);
  VertexPtr get_const(AccessModifiers access);


  int line_num{0};
  const std::vector<Token> tokens;
  DataStream<FunctionPtr> &parsed_os;
  bool is_top_of_the_function_{false};
  decltype(tokens)::const_iterator cur, end;
  std::vector<ClassPtr> class_stack;
  ClassPtr cur_class;               // = class_stack.back()
  std::vector<FunctionPtr> functions_stack;
  FunctionPtr cur_function;         // = functions_stack.back()
  SrcFilePtr processing_file;
};

