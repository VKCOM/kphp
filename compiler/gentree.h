#pragma once
#include "../common.h"

#include "token.h"
#include "operation.h"
#include "data.h"

struct GenTree;
typedef VertexPtr (GenTree::*GetFunc) ();


struct GenTreeCallbackBase {
  virtual void register_function (const FunctionInfo &func) = 0;
  virtual void register_class (const ClassInfo &info) = 0;
  virtual ~GenTreeCallbackBase(){}
};

class GenTree {
public:
  struct AutoLocation {
    int line_num;
    explicit AutoLocation (const GenTree *gen)
      : line_num (gen->line_num) {
    }
  };
  static inline void set_location (VertexPtr v, const AutoLocation &location);

  int line_num;

  GenTree ();

  void init (const vector <Token *> *tokens_new, GenTreeCallbackBase *callback_new);
  void register_function (FunctionInfo info);
  bool in_class();
  bool in_namespace();
  void enter_class (const string &class_name);
  ClassInfo &cur_class();
  void exit_and_register_class (VertexPtr root);
  void enter_function();
  void exit_function();

  bool expect (TokenType tp, const string &msg);
  bool test_expect (TokenType tp);

  void next_cur();
  int open_parent();

  static VertexPtr embrace (VertexPtr v);
  VertexPtr gen_list (VertexPtr node, GetFunc f, TokenType delim, int flags);
  PrimitiveType get_ptype();
  PrimitiveType get_type_help (void);
  VertexPtr get_type_rule_func (void);
  VertexPtr get_type_rule_ (void);
  VertexPtr get_type_rule (void);

  static VertexPtr conv_to (VertexPtr x, PrimitiveType tp, int ref_flag = 0);
  template <PrimitiveType ToT> static VertexPtr conv_to (VertexPtr x);
  static VertexPtr get_actual_value (VertexPtr v);
  static bool has_return (VertexPtr v);
  static void func_force_return (VertexPtr root, VertexPtr val = VertexPtr());
  static void for_each (VertexPtr root, void (*callback) (VertexPtr ));
  static string get_memfunc_prefix (const string &name);

  VertexPtr get_func_param();
  VertexPtr get_foreach_param();
  VertexPtr get_var_name();
  VertexPtr get_var_name_ref();
  VertexPtr get_expr_top();
  VertexPtr get_postfix_expression (VertexPtr res);
  VertexPtr get_unary_op();
  VertexPtr get_binary_op (int bin_op_cur, int bin_op_end, GetFunc next, bool till_ternary);
  VertexPtr get_expression_impl (bool till_ternary);
  VertexPtr get_expression();
  VertexPtr get_statement();
  VertexPtr get_namespace_class();
  VertexPtr get_seq();
  VertexPtr post_process (VertexPtr root);
  bool check_seq_end();
  bool check_statement_end();
  VertexPtr run();

  static bool is_superglobal (const string &s);

  template <Operation EmptyOp> bool gen_list (vector <VertexPtr> *res, GetFunc f, TokenType delim);
  template <Operation Op> VertexPtr get_conv();
  template <Operation Op> VertexPtr get_varg_call();
  template <Operation Op> VertexPtr get_require();
  template <Operation Op, Operation EmptyOp> VertexPtr get_func_call();
  VertexPtr get_short_array();
  VertexPtr get_string();
  VertexPtr get_string_build();
  VertexPtr get_def_value();
  template <PrimitiveType ToT> static VertexPtr conv_to_lval (VertexPtr x);
  template <Operation Op> VertexPtr get_multi_call (GetFunc f);
  VertexPtr get_return();
  VertexPtr get_exit();
  template <Operation Op> VertexPtr get_break_continue();
  VertexPtr get_foreach();
  VertexPtr get_while();
  VertexPtr get_if();
  VertexPtr get_for();
  VertexPtr get_do();
  VertexPtr get_switch();
  VertexPtr get_function (bool anonimous_flag = false, string phpdoc = "");
  VertexPtr get_class();
private:
  const vector <Token *> *tokens;
  GenTreeCallbackBase *callback;
  int in_func_cnt_;
  vector <Token *>::const_iterator cur, end;
  vector <ClassInfo> class_stack;
  string namespace_name;
};
void gen_tree_init();

void php_gen_tree (vector <Token *> *tokens, const string &main_func_name, GenTreeCallbackBase *callback);

template <PrimitiveType ToT>
VertexPtr GenTree::conv_to_lval (VertexPtr x) {
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
  ::set_location (res, x->get_location());
  return res;
}
template <PrimitiveType ToT>
VertexPtr GenTree::conv_to (VertexPtr x) {
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
  ::set_location (res, x->get_location());
  return res;
}

inline void GenTree::set_location (VertexPtr v, const GenTree::AutoLocation &location) {
  v->location.line = location.line_num;
}

static inline bool is_const_int (VertexPtr root) {
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
