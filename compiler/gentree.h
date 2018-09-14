#pragma once

#include "compiler/common.h"
#include "compiler/compiler-core.h"
#include "compiler/data.h"
#include "compiler/operation.h"
#include "compiler/token.h"

class GenTree;

typedef VertexPtr (GenTree::*GetFunc)();


class GenTreeCallback {
  DataStream<FunctionPtr> &os;
public:
  explicit GenTreeCallback(DataStream<FunctionPtr> &os) :
    os(os) {}

  FunctionPtr register_function(const FunctionInfo &info) {
    return G->register_function(info, os);
  }

  void require_function_set(FunctionPtr function) {
    G->require_function_set(fs_function, function->name, FunctionPtr(), os);
  }

  ClassPtr register_class(const ClassInfo &info) {
    return G->register_class(info, os);
  }

  ClassPtr get_class_by_name(const string &class_name) {
    return G->get_class(class_name);
  }
};


class GenTree {
public:
  struct AutoLocation {
    int line_num;

    explicit AutoLocation(const GenTree *gen) :
      line_num(gen->line_num) {
    }
  };

  static inline void set_location(VertexPtr v, const AutoLocation &location);

  GenTree();

  void init(const vector<Token *> *tokens_new, const string &context, GenTreeCallback &callback_new);
  FunctionPtr register_function(FunctionInfo info);
  bool in_class();
  bool in_namespace() const;
  void enter_class(const string &class_name, Token *phpdoc_token);
  ClassInfo &cur_class();
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
  PrimitiveType get_type_help(void);
  VertexPtr get_type_rule_func(void);
  VertexPtr get_type_rule_(void);
  VertexPtr get_type_rule(void);

  static VertexPtr conv_to(VertexPtr x, PrimitiveType tp, int ref_flag = 0);
  template<PrimitiveType ToT>
  static VertexPtr conv_to(VertexPtr x);
  static VertexPtr get_actual_value(VertexPtr v);
  static bool has_return(VertexPtr v);
  static void func_force_return(VertexPtr root, VertexPtr val = VertexPtr());
  static void for_each(VertexPtr root, void (*callback)(VertexPtr));
  VertexPtr create_vertex_this(const AutoLocation &location, bool with_type_rule = false);
  void patch_func_constructor(VertexAdaptor<op_function> func, const ClassInfo &cur_class);
  void patch_func_add_this(vector<VertexPtr> &params_next, const AutoLocation &func_location);
  FunctionPtr create_default_constructor(const ClassInfo &cur_class);

  VertexPtr get_func_param();
  VertexPtr get_foreach_param();
  VertexPtr get_var_name();
  VertexPtr get_var_name_ref();
  VertexPtr get_expr_top();
  VertexPtr get_postfix_expression(VertexPtr res);
  VertexPtr get_unary_op();
  VertexPtr get_binary_op(int bin_op_cur, int bin_op_end, GetFunc next, bool till_ternary);
  VertexPtr get_expression_impl(bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement(Token *phpdoc_token = nullptr);
  VertexPtr get_vars_list(Token *phpdoc_token, OperationExtra extra_type);
  VertexPtr get_namespace_class();
  VertexPtr get_use();
  VertexPtr get_seq();
  VertexPtr post_process(VertexPtr root);
  bool check_seq_end();
  bool check_statement_end();
  VertexPtr run();

  static bool is_superglobal(const string &s);

  template<Operation EmptyOp>
  bool gen_list(vector<VertexPtr> *res, GetFunc f, TokenType delim);
  template<Operation Op>
  VertexPtr get_conv();
  template<Operation Op>
  VertexPtr get_varg_call();
  template<Operation Op>
  VertexPtr get_require();
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
  VertexPtr get_function(bool anonimous_flag = false, Token *phpdoc_token = nullptr, AccessType access_type = access_nonmember);
  VertexPtr get_class(Token *phpdoc_token);

private:
  VertexPtr create_function_vertex_with_flags(VertexPtr name, VertexPtr params, VertexPtr flags, TokenType type, VertexPtr cmd, bool is_constructor);
  void set_extra_type(VertexPtr vertex, AccessType access_type) const;
  void fill_info_about_class(FunctionInfo &info);
  void add_parent_function_to_descendants_with_context(FunctionInfo info, AccessType access_type, const vector<VertexPtr> &params_next);
  VertexPtr generate_function_with_parent_call(FunctionInfo info, const string &real_name, const vector<VertexPtr> &params_next);
  string get_name_for_new_function_with_parent_call(const FunctionInfo &info, const string &real_name);

public:
  int line_num;

private:
  const vector<Token *> *tokens;
  GenTreeCallback *callback;
  int in_func_cnt_;
  bool is_top_of_the_function_;
  vector<Token *>::const_iterator cur, end;
  vector<ClassInfo> class_stack;
  string namespace_name;
  string class_context;
  ClassPtr context_class_ptr;
  string class_extends;
  map<string, string> namespace_uses;
};

void gen_tree_init();

void php_gen_tree(vector<Token *> *tokens, const string &context, const string &main_func_name, GenTreeCallback &callback);

template<PrimitiveType ToT>
VertexPtr GenTree::conv_to_lval(VertexPtr x) {
  VertexPtr res;
  switch (ToT) {
    case tp_array: {
      CREATE_VERTEX (v, op_conv_array_l, x);
      res = v;
      break;
    }
    case tp_int: {
      CREATE_VERTEX (v, op_conv_int_l, x);
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
      CREATE_VERTEX (v, op_conv_int, x);
      res = v;
      break;
    }
    case tp_bool: {
      CREATE_VERTEX (v, op_conv_bool, x);
      res = v;
      break;
    }
    case tp_string: {
      CREATE_VERTEX (v, op_conv_string, x);
      res = v;
      break;
    }
    case tp_float: {
      CREATE_VERTEX (v, op_conv_float, x);
      res = v;
      break;
    }
    case tp_array: {
      CREATE_VERTEX (v, op_conv_array, x);
      res = v;
      break;
    }
    case tp_UInt: {
      CREATE_VERTEX (v, op_conv_uint, x);
      res = v;
      break;
    }
    case tp_Long: {
      CREATE_VERTEX (v, op_conv_long, x);
      res = v;
      break;
    }
    case tp_ULong: {
      CREATE_VERTEX (v, op_conv_ulong, x);
      res = v;
      break;
    }
    case tp_regexp: {
      CREATE_VERTEX (v, op_conv_regexp, x);
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
      return is_const_int(root.as<meta_op_unary_op>()->expr());
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
      return is_const_int(root.as<meta_op_binary_op>()->lhs()) && is_const_int(root.as<meta_op_binary_op>()->rhs());
    default:
      break;
  }
  return false;
}
