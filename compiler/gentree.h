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
  struct AutoLocation {
    int line_num;

    explicit AutoLocation(int line_num) :
      line_num(line_num) {}

    explicit AutoLocation(const GenTree *gen) :
      line_num(gen->line_num) {
    }
  };

  static inline void set_location(VertexPtr v, const AutoLocation &location);
  VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix, PrimitiveType tp = tp_Unknown);
  static VertexAdaptor<op_var> create_superlocal_var(const std::string& name_prefix, FunctionPtr cur_function, PrimitiveType tp = tp_Unknown);
  static VertexAdaptor<op_switch> create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition, std::vector<VertexPtr> &&cases);

  GenTree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

  VertexPtr generate_constant_field_class_value();

  bool test_expect(TokenType tp);

  void next_cur();
  int open_parent();
  void skip_phpdoc_tokens();

  static VertexAdaptor<op_seq> embrace(VertexPtr v);
  PrimitiveType get_func_param_type_help();
  VertexAdaptor<meta_op_type_rule> get_type_rule();

  static VertexPtr conv_to(VertexPtr x, PrimitiveType tp, bool ref_flag = 0);
  template<PrimitiveType ToT>
  static VertexPtr conv_to(VertexPtr x);
  static VertexPtr get_actual_value(VertexPtr v);
  static const std::string *get_constexpr_string(VertexPtr v);
  static int get_id_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr);
  static VertexPtr get_call_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr);

  static void func_force_return(VertexAdaptor<op_function> func, VertexPtr val = VertexPtr());
  VertexAdaptor<op_ternary> create_ternary_op_vertex(VertexPtr left, VertexPtr right, VertexPtr third);
  VertexAdaptor<op_type_expr_class> create_type_help_class_vertex(vk::string_view klass_name);
  static VertexAdaptor<op_type_expr_class> create_type_help_class_vertex(ClassPtr klass);
  static VertexAdaptor<op_type_expr_type> create_type_help_vertex(PrimitiveType type, const std::vector<VertexPtr> &children = {});

  VertexAdaptor<op_func_param> get_func_param_without_callbacks(bool from_callback = false);
  VertexAdaptor<op_func_param> get_func_param_from_callback();
  VertexAdaptor<meta_op_func_param> get_func_param();
  VertexAdaptor<op_foreach_param> get_foreach_param();
  VertexAdaptor<op_var> get_var_name();
  VertexAdaptor<op_var> get_var_name_ref();
  VertexPtr get_expr_top(bool was_arrow);
  VertexPtr get_postfix_expression(VertexPtr res);
  VertexPtr get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary);
  VertexPtr get_binary_op(int op_priority_cur, bool till_ternary);
  VertexPtr get_expression_impl(bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement(const vk::string_view &phpdoc_str = vk::string_view{});
  VertexPtr get_instance_var_list(const vk::string_view &phpdoc_str, FieldModifiers modifiers);
  void get_traits_uses();
  void get_use();
  void get_seq(std::vector<VertexPtr> &seq_next);
  VertexPtr get_seq();

  void parse_declare_at_top_of_file();
  void parse_namespace_and_uses_at_top_of_file();
  bool check_seq_end();
  bool check_statement_end();
  void run();

  template<Operation EmptyOp, class FuncT, class ResultType = typename vk::function_traits<FuncT>::ResultType>
  bool gen_list(std::vector<ResultType> *res, FuncT f, TokenType delim);
  template<Operation Op>
  VertexPtr get_conv();
  VertexPtr get_require(bool once);
  template<Operation Op, Operation EmptyOp>
  VertexAdaptor<Op> get_func_call();
  VertexPtr get_short_array();
  VertexPtr get_string();
  VertexPtr get_string_build();
  VertexPtr get_def_value();
  template<PrimitiveType ToT>
  static VertexPtr conv_to_lval(VertexPtr x);
  template<Operation Op, class FuncT, class ResultType = typename vk::function_traits<FuncT>::ResultType>
  VertexAdaptor<op_seq> get_multi_call(FuncT &&f, bool parenthesis = false);
  VertexPtr get_return();
  template<Operation Op>
  VertexPtr get_break_continue();
  VertexPtr get_foreach();
  VertexPtr get_while();
  VertexPtr get_if();
  VertexPtr get_for();
  VertexPtr get_do();
  VertexPtr get_switch();
  bool parse_function_uses(std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda);
  static bool check_uses_and_args_are_not_intersecting(const std::vector<VertexAdaptor<op_func_param>> &uses, const VertexRange &params);
  VertexPtr get_anonymous_function(bool is_static = false);
  VertexPtr get_function(const vk::string_view &phpdoc_str, FunctionModifiers modifiers, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda = nullptr);

  ClassMemberModifiers parse_class_member_modifier_mask();
  VertexPtr get_class_member(const vk::string_view &phpdoc_str);

  static LambdaGenerator generate_anonymous_class(VertexAdaptor<op_function> function,
                                                  FunctionPtr parent_function,
                                                  bool is_static,
                                                  std::vector<VertexAdaptor<op_func_param>> &&uses_of_lambda,
                                                  FunctionPtr already_created_function = FunctionPtr{});

  static VertexAdaptor<op_func_call> generate_call_on_instance_var(VertexPtr instance_var, FunctionPtr function, vk::string_view function_name);
  static VertexAdaptor<op_func_call> gen_constructor_call_with_args(ClassPtr allocated_class, std::vector<VertexPtr> args);

  VertexPtr get_class(const vk::string_view &phpdoc_str, ClassType class_type);
  void parse_extends_implements();

private:
  std::string get_typehint();

  VertexAdaptor<op_func_param_list> parse_cur_function_param_list();

  VertexPtr get_static_field_list(const vk::string_view &phpdoc_str, FieldModifiers modifiers);

  void require_lambdas() {
    for (auto &generator : lambda_generators) {
      generator.require(parsed_os);
    }
    lambda_generators.clear();
  }

  VertexPtr get_const_after_explicit_access_modifier(AccessModifiers access);
  VertexPtr get_const(AccessModifiers access);

public:
  int line_num;

private:
  const vector<Token> tokens;
  DataStream<FunctionPtr> &parsed_os;
  bool is_top_of_the_function_;
  decltype(tokens)::const_iterator cur, end;
  vector<ClassPtr> class_stack;
  ClassPtr cur_class;               // = class_stack.back()
  vector<FunctionPtr> functions_stack;
  FunctionPtr cur_function;         // = functions_stack.back()
  SrcFilePtr processing_file;
  std::vector<LambdaGenerator> lambda_generators;
};

void php_gen_tree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

template<PrimitiveType ToT>
VertexPtr GenTree::conv_to_lval(VertexPtr x) {
  VertexPtr res;
  switch (ToT) {
    case tp_array: {
      res = VertexAdaptor<op_conv_array_l>::create(x);
      break;
    }
    case tp_int: {
      res = VertexAdaptor<op_conv_int_l>::create(x);
      break;
    }
    case tp_string: {
      res = VertexAdaptor<op_conv_string_l>::create(x);
      break;
    }
  }
  ::set_location(res, x->get_location());
  return res;
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

    case tp_UInt:
      return VertexAdaptor<op_conv_uint>::create(x).set_location(x);

    case tp_Long:
      return VertexAdaptor<op_conv_long>::create(x).set_location(x);

    case tp_ULong:
      return VertexAdaptor<op_conv_ulong>::create(x).set_location(x);

    case tp_regexp:
      return VertexAdaptor<op_conv_regexp>::create(x).set_location(x);

    default:
      return x;
  }
}

inline void GenTree::set_location(VertexPtr v, const GenTree::AutoLocation &location) {
  v->location.line = location.line_num;
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
