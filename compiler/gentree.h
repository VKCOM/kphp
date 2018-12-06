#pragma once

#include "compiler/common.h"
#include "compiler/compiler-core.h"
#include "compiler/operation.h"
#include "compiler/token.h"

class GenTree;

typedef VertexPtr (GenTree::*GetFunc)();


class GenTree {
private:
  class SetFunctionsStackGuard {
  public:
    SetFunctionsStackGuard(GenTree *g) :
      prev_functions_stack(g->functions_stack),
      g(g) {
      g->functions_stack = &new_functions_stack;
    }

    void reset() {
      g->functions_stack = prev_functions_stack;
    }

    ~SetFunctionsStackGuard() {
      g->functions_stack = prev_functions_stack;
    }

    std::vector<FunctionPtr> *prev_functions_stack = nullptr;
    std::vector<FunctionPtr> new_functions_stack;
  private:
    GenTree *g;
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

  GenTree(const vector<Token *> *tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

  static FunctionPtr create_and_register_function(
    VertexPtr root,
    const std::string &namespace_name,
    ClassPtr context_class,
    AccessType access_type,
    FunctionData::func_type_t type,
    DataStream<FunctionPtr> &os);
  
  VertexPtr generate_constant_field_class(VertexPtr root);
  void exit_and_register_class(VertexPtr root);
  void enter_function();
  void exit_function();

  bool test_expect(TokenType tp);

  void next_cur();
  int open_parent();
  void skip_phpdoc_tokens();

  static VertexPtr embrace(VertexPtr v);
  PrimitiveType get_ptype();
  PrimitiveType get_func_param_type_help();
  VertexPtr get_type_rule_func();
  VertexPtr get_type_rule_();
  VertexPtr get_type_rule();

  static VertexPtr conv_to(VertexPtr x, PrimitiveType tp, int ref_flag = 0);
  template<PrimitiveType ToT>
  static VertexPtr conv_to(VertexPtr x);
  static VertexPtr get_actual_value(VertexPtr v);
  static bool has_return(VertexPtr v);
  static void func_force_return(VertexPtr root, VertexPtr val = VertexPtr());
  static void for_each(VertexPtr root, void (*callback)(VertexPtr));
  static VertexPtr create_vertex_this(const AutoLocation &location, ClassPtr cur_class, bool with_type_rule = false);
  static void patch_func_constructor(VertexAdaptor<op_function> func, ClassPtr cur_class, AutoLocation location);
  static void patch_func_add_this(vector<VertexPtr> &params_next, const AutoLocation &func_location, ClassPtr cur_class);
  static VertexPtr create_ternary_op_vertex(VertexPtr left, VertexPtr right, VertexPtr third);
  void create_default_constructor(ClassPtr context_class, ClassPtr cur_class, AutoLocation location) const;
  static void create_constructor_with_args(ClassPtr context_class, ClassPtr cur_class,
                                           AutoLocation location, VertexAdaptor<op_func_param_list> params,
                                           DataStream<FunctionPtr> &os);

  VertexPtr get_func_param_without_callbacks(bool from_callback = false);
  VertexPtr get_func_param_from_callback();
  VertexPtr get_func_param();
  VertexPtr get_foreach_param();
  VertexPtr get_var_name();
  VertexPtr get_var_name_ref();
  VertexPtr get_expr_top();
  VertexPtr get_postfix_expression(VertexPtr res);
  VertexPtr get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary);
  VertexPtr get_binary_op(int op_priority_cur, bool till_ternary);
  VertexPtr get_expression_impl(bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement(Token *phpdoc_token = nullptr);
  VertexPtr get_instance_var_list(Token *phpdoc_token, AccessType access_type);
  VertexPtr get_namespace_class();
  VertexPtr get_use();
  VertexPtr get_seq();
  bool check_seq_end();
  bool check_statement_end();
  VertexPtr run();

  template<Operation EmptyOp>
  bool gen_list(vector<VertexPtr> *res, GetFunc f, TokenType delim);
  template<Operation Op>
  VertexPtr get_conv();
  template<Operation Op>
  VertexPtr get_varg_call();
  VertexPtr get_require();
  VertexPtr get_require_once();
  template<Operation Op, Operation EmptyOp>
  VertexPtr get_func_call();
  VertexPtr get_short_array();
  VertexPtr get_string();
  VertexPtr get_string_build();
  VertexPtr get_def_value();
  template<PrimitiveType ToT>
  static VertexPtr conv_to_lval(VertexPtr x);
  template<Operation Op>
  VertexPtr get_multi_call(GetFunc f);
  VertexPtr get_return();
  VertexPtr get_exit();
  template<Operation Op>
  VertexPtr get_break_continue();
  VertexPtr get_foreach();
  VertexPtr get_while();
  VertexPtr get_if();
  VertexPtr get_for();
  VertexPtr get_do();
  VertexPtr get_switch();
  bool parse_function_specifiers(VertexPtr flags);
  bool parse_function_uses(std::vector<VertexPtr> *uses_of_lambda);
  static bool check_uses_and_args_are_not_intersect(const std::vector<VertexPtr> &uses, const std::vector<VertexPtr> &params);
  VertexPtr get_anonymous_function();
  VertexPtr get_function(Token *phpdoc_token = nullptr, AccessType access_type = access_nonmember, std::vector<VertexPtr> *uses_of_lambda = nullptr);

  static void add_this_to_captured_variables_in_lambda_body(VertexPtr &cmd, ClassPtr lambda_class);
  static VertexAdaptor<op_function> generate__invoke_method(ClassPtr cur_class, const VertexAdaptor<op_function> &function);
  static VertexPtr generate_constructor_call(ClassPtr cur_class);
  static VertexPtr generate_anonymous_class(VertexAdaptor<op_function> function,
                                            DataStream<FunctionPtr> &os,
                                            AccessType cur_access_type,
                                            std::vector<VertexPtr> &&uses_of_lambda);

  VertexPtr get_class(Token *phpdoc_token);

  static std::string concat_namespace_class_function_names(const std::string &namespace_name,
                                                           const std::string &class_name,
                                                           const std::string &function_name);


private:
  VertexPtr parse_function_declaration(AccessType access_type,
                                       std::vector<VertexPtr> *uses_of_lambda,
                                       VertexAdaptor<op_func_param_list> &params,
                                       VertexPtr &flags,
                                       bool &is_constructor);

  VertexPtr create_function_vertex_with_flags(VertexPtr name, VertexPtr params, VertexPtr flags, TokenType type, VertexPtr cmd, bool is_constructor);

  static void add_parent_function_to_child_class_with_context(VertexAdaptor<op_function> root, ClassPtr parent_class, ClassPtr child_class, AccessType access_type, DataStream<FunctionPtr> &os);
  static VertexPtr generate_function_with_parent_call(VertexAdaptor<op_function> root, ClassPtr parent_class, ClassPtr child_class, const string &function_local_name);
  static void add_namespace_and_context_to_function_name(ClassPtr cur_class, ClassPtr context_class, std::string &function_name);

  /**
   * @param full_name full method name including namespace and context encoded
   * @return `method_name` if `Namespace$ClassName$$method_name$$Context` was passed
   */
  static std::string get_real_name_from_full_method_name(const std::string &full_name);

  bool in_namespace() const;

  VertexPtr get_static_field_list(Token *phpdoc_token, AccessType access_type);

public:
  int line_num;

private:
  const vector<Token *> *tokens;
  DataStream<FunctionPtr> &parsed_os;
  int in_func_cnt_;
  bool is_top_of_the_function_;
  vector<Token *>::const_iterator cur, end;
  vector<ClassPtr> class_stack;
  std::vector<FunctionPtr> *functions_stack = nullptr;
  AccessType cur_acccess_type = AccessType::access_nonmember;
  ClassPtr cur_class;               // = class_stack.back(), просто обращений очень много
  SrcFilePtr processing_file;
  ClassPtr context_class;
};

void php_gen_tree(vector<Token *> *tokens, SrcFilePtr file, DataStream<FunctionPtr> &os);

template<PrimitiveType ToT>
VertexPtr GenTree::conv_to_lval(VertexPtr x) {
  VertexPtr res;
  switch (ToT) {
    case tp_array: {
      auto v = VertexAdaptor<op_conv_array_l>::create(x);
      res = v;
      break;
    }
    case tp_int: {
      auto v = VertexAdaptor<op_conv_int_l>::create(x);
      res = v;
      break;
    }
  }
  ::set_location(res, x->get_location());
  return res;
}

template<PrimitiveType ToT>
VertexPtr GenTree::conv_to(VertexPtr x) {
  VertexPtr res;
  switch (ToT) {
    case tp_int: {
      auto v = VertexAdaptor<op_conv_int>::create(x);
      res = v;
      break;
    }
    case tp_bool: {
      auto v = VertexAdaptor<op_conv_bool>::create(x);
      res = v;
      break;
    }
    case tp_string: {
      auto v = VertexAdaptor<op_conv_string>::create(x);
      res = v;
      break;
    }
    case tp_float: {
      auto v = VertexAdaptor<op_conv_float>::create(x);
      res = v;
      break;
    }
    case tp_array: {
      auto v = VertexAdaptor<op_conv_array>::create(x);
      res = v;
      break;
    }
    case tp_UInt: {
      auto v = VertexAdaptor<op_conv_uint>::create(x);
      res = v;
      break;
    }
    case tp_Long: {
      auto v = VertexAdaptor<op_conv_long>::create(x);
      res = v;
      break;
    }
    case tp_ULong: {
      auto v = VertexAdaptor<op_conv_ulong>::create(x);
      res = v;
      break;
    }
    case tp_regexp: {
      auto v = VertexAdaptor<op_conv_regexp>::create(x);
      res = v;
      break;
    }
  }
  ::set_location(res, x->get_location());
  return res;
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
  VertexPtr actual_value = GenTree::get_actual_value(v);
  return actual_value->type() == op_int_const &&
         parse_int_from_string(actual_value) >= 0;
}
