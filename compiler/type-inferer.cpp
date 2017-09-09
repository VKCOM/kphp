#include "compiler/type-inferer.h"

void init_functions_tinf_nodes (FunctionPtr function) {
  assert (function->tinf_state == 1);
  VertexRange params = get_function_params (function->root);
  function->tinf_nodes.resize (params.size() + 1);
  for (int i = 0; i < (int)function->tinf_nodes.size(); i++) {
    function->tinf_nodes[i].param_i = i - 1;
    function->tinf_nodes[i].function_ = function;
    if (i && function->type() != FunctionData::func_extern) {
      function->tinf_nodes[i].var_ = function->param_ids[i - 1];
    }
  }

  __sync_synchronize();
  function->tinf_state = 2;
}

tinf::Node *get_tinf_node (FunctionPtr function, int id) {
  if (function->tinf_state == 0) {
    if (__sync_bool_compare_and_swap (&function->tinf_state, 0, 1)) {
      init_functions_tinf_nodes (function);
    }
  }
  while (function->tinf_state != 2) {
    __sync_synchronize();
  }

  assert (-1 <= id && id + 1 < (int)function->tinf_nodes.size());
  return &function->tinf_nodes[id + 1];
}

tinf::Node *get_tinf_node (VertexPtr vertex) {
  return &vertex->tinf_node;
}
tinf::Node *get_tinf_node (VarPtr vertex) {
  return &vertex->tinf_node;
}

const TypeData *get_type (VertexPtr vertex, tinf::TypeInferer *inferer) {
  return inferer->get_type (get_tinf_node (vertex));
}

const TypeData *get_type (VarPtr var, tinf::TypeInferer *inferer) {
  return inferer->get_type (get_tinf_node (var));
}

const TypeData *get_type (FunctionPtr function, int id, tinf::TypeInferer *inferer) {
  return inferer->get_type (get_tinf_node (function, id));
}

const TypeData *fast_get_type (VertexPtr vertex) {
  return get_tinf_node (vertex)->get_type();
}
const TypeData *fast_get_type (VarPtr var) {
  return get_tinf_node (var)->get_type();
}
const TypeData *fast_get_type (FunctionPtr function, int id) {
  return get_tinf_node (function, id)->get_type();
}

/*** Restrictions ***/
RestrictionIsset::RestrictionIsset (tinf::Node *a) :
  a_ (a) {
    //empty
  }

const char *RestrictionIsset::get_description() {
  return desc.c_str();
}

static string remove_after_tab(const string& s){
  string ns = "";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\t') {
      break;
    } else {
      ns += s[i];
    }
  }
  return ns;
}

void RestrictionIsset::find_dangerous_isset_warning (const vector <tinf::Node *> &bt, tinf::Node *node, const string &msg __attribute__((unused))) {
  stringstream ss;
  ss << "isset, !==, ===, is_array or similar function result may differ from PHP\n" <<
        " Probably, this happened because " << remove_after_tab(node->get_description()) << " of type "<<
        type_out(node->get_type()) <<" can't be null in KPHP, while it can be in PHP\n" <<
        " Chain of assignments:\n";

  FOREACH (bt, it) {
    ss << "  " << (*it)->get_description() << "\n";
  }
  ss << "  " << node->get_description() << "\n";
  desc = ss.str();
}

bool RestrictionIsset::isset_is_dangerous (int isset_flags, const TypeData *tp) {
  PrimitiveType ptp = tp->ptype();
  bool res = false;
  if (isset_flags & ifi_isset) {
    return ptp != tp_var;
  }
  res |= (ptp == tp_array) && (isset_flags & ifi_is_array);
  res |= (ptp == tp_bool || tp->use_or_false()) &&
    (isset_flags & (ifi_is_bool | ifi_is_scalar));
  res |= (ptp == tp_int) &&
    (isset_flags & (ifi_is_scalar | ifi_is_numeric | ifi_is_integer | ifi_is_long));
  res |= (ptp == tp_float) &&
    (isset_flags & (ifi_is_scalar | ifi_is_numeric | ifi_is_float |
                    ifi_is_double | ifi_is_real));
  res |= (ptp == tp_string) &&
    (isset_flags & (ifi_is_scalar | ifi_is_string));
  return res;
}

bool RestrictionIsset::find_dangerous_isset_dfs (int isset_flags, tinf::Node *node,
    vector <tinf::Node *> *bt) {
  if ((node->isset_was & isset_flags) == isset_flags) {
    return false;
  }
  node->isset_was |= isset_flags;

  tinf::TypeNode *type_node = dynamic_cast <tinf::TypeNode *> (node);
  if (type_node != NULL) {
    return false;
  }

  tinf::ExprNode *expr_node = dynamic_cast <tinf::ExprNode *> (node);
  if (expr_node != NULL) {
    VertexPtr v = expr_node->get_expr();
    if (v->type() == op_index && isset_is_dangerous (isset_flags, node->get_type())) {
      node->isset_was = -1;
      find_dangerous_isset_warning (*bt, node, "[index]");
      return true;
    }
    if (v->type() == op_var) {
      VarPtr from_var = v.as <op_var>()->get_var_id();
      if (from_var.not_null() && from_var->get_uninited_flag() && isset_is_dangerous (isset_flags, node->get_type())) {
        node->isset_was = -1;
        find_dangerous_isset_warning (*bt, node, "[uninited varialbe]");
        return true;
      }

      bt->push_back (node);
      if (find_dangerous_isset_dfs (isset_flags, get_tinf_node (from_var), bt)) {
        return true;
      }
      bt->pop_back();
    }
    if (v->type() == op_func_call) {
      FunctionPtr func = v.as <op_func_call>()->get_func_id();
      bt->push_back (node);
      if (find_dangerous_isset_dfs (isset_flags, get_tinf_node (func, -1), bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }

  tinf::VarNode *var_node = dynamic_cast <tinf::VarNode *> (node);
  if (var_node != NULL) {
    VarPtr from_var = var_node->get_var();
    FOREACH (var_node->next_range(), it) {
      tinf::Edge *e = *it;
      if (all(*(e->from_at)).size() != 0) {
        continue;
      }

      tinf::Node *to_node = e->to;

      /*** function f(&$a){}; f($b) fix ***/
      if (from_var.not_null()) {
        VarPtr to_var;
        tinf::VarNode *to_var_node = dynamic_cast <tinf::VarNode *> (to_node);
        if (to_var_node != NULL) {
          to_var = to_var_node->get_var();
        }
        if (to_var.not_null() && to_var->type() == VarData::var_param_t && 
            !(to_var->holder_func == from_var->holder_func)) {
          continue;
        }
      }

      bt->push_back (node);
      if (find_dangerous_isset_dfs (isset_flags, to_node, bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }
  return false;
}

int RestrictionIsset::check_impl() {
  vector <tinf::Node *> bt;
  return find_dangerous_isset_dfs (a_->isset_flags, a_, &bt);
}

void tinf::VarNode::recalc (tinf::TypeInferer *inferer) {
  VarNodeRecalc f(this, inferer);
  f.run();
}

void tinf::ExprNode::recalc (tinf::TypeInferer *inferer) {
  ExprNodeRecalc f(this, inferer);
  f.run();
}

string tinf::VarNode::get_description() {
  stringstream ss;
  if (function_.is_null()) {
    if (var_.not_null() && var_->holder_func.not_null()) {
      function_ = var_->holder_func;
    }
  }
  if (param_i == -2) {
    ss << "[$" << var_->name << "]";
  } else if (param_i == -1) {
    ss << "[return .]";
  } else {
    ss << "[arg #" << int_to_str (param_i) << " ($" << var_->name << ")]";
  }
  if (function_.not_null()) {
    ss << "\tat [function = " << function_->name << "]";
  }
  return ss.str();
}

string tinf::TypeNode::get_description() {
  return "type[" + type_out (type_) + "]";
}
static string get_expr_description (VertexPtr expr) {
  if (expr->type() == op_var) {
    return "$" + expr.as <op_var>()->get_var_id()->name;
  } else if (expr->type() == op_func_call) {
    return expr.as <op_func_call>()->get_func_id()->name + "(...)";
  } else if (expr->type() == op_index) {
    string suff = "";
    while (expr->type() == op_index) {
      suff += "[.]";
      expr = expr.as <op_index>()->array();
    }
    return get_expr_description (expr) + suff;
  }
  return OpInfo::str (expr->type());
}
string tinf::ExprNode::get_description() {
  stringstream ss;
  ss << "expr[";
  ss << get_expr_description (expr_);
  ss << "]";
  ss << "\tat [" << stage::to_str (expr_->get_location()) << "]";
  return ss.str();
}

const TypeData *NodeRecalc::new_type() {
  return new_type_;
}

bool NodeRecalc::auto_edge_flag() {
  return false;
}
void NodeRecalc::add_dependency_impl (tinf::Node *from, tinf::Node *to) {
  tinf::Edge *e = new tinf::Edge();
  e->from = from;
  e->to = to;
  e->from_at = NULL;
  inferer_->add_edge (e);
  inferer_->add_node (e->to);
}
void NodeRecalc::add_dependency (const RValue &rvalue) {
  if (auto_edge_flag() && rvalue.node != NULL) {
    add_dependency_impl (node_, rvalue.node);
  }
}
void NodeRecalc::set_lca_at (const MultiKey *key, const RValue &rvalue) {
  if (new_type_->error_flag()) {
    return;
  }
  const TypeData *type = NULL;
  if (rvalue.node != NULL) {
    if (auto_edge_flag()) {
      add_dependency_impl (node_, rvalue.node);
    }
    __sync_synchronize();
    type = rvalue.node->get_type();
  } else if (rvalue.type != NULL) {
    type = rvalue.type;
  } else {
    kphp_fail();
  }

  if (rvalue.key != NULL) {
    type = type->const_read_at (*rvalue.key);
  }
  if (key == NULL) {
    key = &MultiKey::any_key (0);
  }

  if (type->error_flag()) {
    return;
  }
  if (types_stack.empty()) {
    new_type_->set_lca_at (*key, type, !rvalue.drop_or_false);
  } else {
    types_stack.back()->set_lca_at (*key, type, !rvalue.drop_or_false);
  }
  if (new_type_->error_flag()) {
    kphp_error (0, dl_pstr ("Type Error [%s]\n", node_->get_description().c_str()));
  }
}
void NodeRecalc::set_lca_at (const MultiKey *key, VertexPtr expr) {
  set_lca_at (key, as_rvalue (expr));
}
void NodeRecalc::set_lca_at (const MultiKey *key, PrimitiveType ptype) {
  set_lca_at (key, as_rvalue (ptype));
}

void NodeRecalc::set_lca (const RValue &rvalue) {
  set_lca_at (NULL, rvalue);
}
void NodeRecalc::set_lca (PrimitiveType ptype) {
  set_lca (as_rvalue (ptype));
}
void NodeRecalc::set_lca (FunctionPtr function, int id) {
  return set_lca (as_rvalue (function, id));
}
void NodeRecalc::set_lca (VertexPtr vertex, const MultiKey *key /* = NULL*/) {
  return set_lca (as_rvalue (vertex, key));
}
void NodeRecalc::set_lca (const TypeData *type, const MultiKey *key /* = NULL*/) {
  return set_lca (as_rvalue (type, key));
}
void NodeRecalc::set_lca (VarPtr var) {
  return set_lca (as_rvalue (var));
}

NodeRecalc::NodeRecalc (tinf::Node *node, tinf::TypeInferer *inferer) :
  node_ (node),
  inferer_ (inferer)
{
}

void NodeRecalc::on_changed() {
  __sync_synchronize();

  node_->set_type (new_type_);
  new_type_ = NULL;

  __sync_synchronize();

  AutoLocker <Lockable *> locker (node_);
  FOREACH (node_->rev_next_range(), it) {
    tinf::Edge *e = *it;
    inferer_->recalc_node (e->from);
  }
}

void NodeRecalc::run() {
  const TypeData *old_type = node_->get_type();
  new_type_ = old_type->clone();

  TypeData::upd_generation (new_type_->generation());
  TypeData::generation_t old_generation = new_type_->generation();
  TypeData::inc_generation();

  do_recalc();
  new_type_->fix_inf_array();

  //fprintf (stderr, "upd %d %p %s %s->%s %d\n", get_thread_id(), node_, node_->get_description().c_str(), type_out (node_->get_type()).c_str(), type_out (new_type_).c_str(), new_type_->generation() != old_generation);
  if (new_type_->generation() != old_generation) {
    on_changed();
  }

  delete new_type_;
}

void NodeRecalc::push_type() {
  types_stack.push_back (TypeData::get_type (tp_Unknown)->clone());
}
TypeData *NodeRecalc::pop_type() {
  TypeData *result = types_stack.back();
  types_stack.pop_back();
  return result;
}

VarNodeRecalc::VarNodeRecalc (tinf::VarNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc (node, inferer) {
  }

void VarNodeRecalc::do_recalc() {
  //fprintf (stderr, "recalc var %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());

  if (inferer_->is_finished()) {
    kphp_error (0, dl_pstr ("%s: %d\n", node_->get_description().c_str(), node_->recalc_cnt_));
    kphp_fail();
  }
  FOREACH (node_->next_range(), it) {
    tinf::Edge *e = *it;
    set_lca_at (e->from_at, e->to);
    inferer_->add_node (e->to);
  }
}
template <PrimitiveType tp>
void ExprNodeRecalc::recalc_ptype() {

  set_lca (TypeData::get_type (tp));
}

void ExprNodeRecalc::recalc_require (VertexAdaptor <op_require> require) {
  FunctionPtr last_function = require->back()->get_func_id();
  set_lca (last_function, -1);
}

void ExprNodeRecalc::recalc_ternary (VertexAdaptor <op_ternary> ternary) {
  set_lca (ternary->true_expr());
  set_lca (ternary->false_expr());
}

void ExprNodeRecalc::apply_type_rule_func (VertexAdaptor <op_type_rule_func> func, VertexPtr expr) {
  if (func->str_val == "lca") {
    FOREACH_VERTEX (func->args(), i) {
      //TODO: is it hack?
      apply_type_rule (*i, expr);
    }
    return;
  }
  if (func->str_val == "OrFalse") {
    if (kphp_error (!func->args().empty(), "OrFalse with no arguments")) {
      recalc_ptype <tp_Error>();
    } else {
      apply_type_rule (func->args()[0], expr);
      recalc_ptype <tp_False> ();
    }
    return;
  }
  kphp_error (0, dl_pstr ("unknown type_rule function [%s]", func->str_val.c_str()));
  recalc_ptype <tp_Error>();
}
void ExprNodeRecalc::apply_type_rule_type (VertexAdaptor <op_type_rule> rule, VertexPtr expr) {
  set_lca (rule->type_help);
  if (!rule->empty()) {
    push_type();
    apply_type_rule (rule->args()[0], expr);
    TypeData *tmp = pop_type();
    set_lca_at (&MultiKey::any_key (1), tmp);
    delete tmp;
  }
}
void ExprNodeRecalc::apply_arg_ref (VertexAdaptor <op_arg_ref> arg, VertexPtr expr) {
  int i = arg->int_val;
  if (expr.is_null() || i < 1 || expr->type() != op_func_call ||
      i > (int)get_function_params (expr->get_func_id()->root).size()) {
    kphp_error (0, "error in type rule");
    recalc_ptype <tp_Error>();
  }

  VertexRange call_args = expr.as <op_func_call>()->args();
  if (i - 1 < (int)call_args.size()) {
    set_lca (call_args[i - 1]);
  }
}
void ExprNodeRecalc::apply_index (VertexAdaptor <op_index> index, VertexPtr expr) {
  push_type ();
  apply_type_rule (index->array(), expr);
  TypeData *type = pop_type();
  set_lca (type, &MultiKey::any_key (1));
  delete type;
}

void ExprNodeRecalc::apply_type_rule (VertexPtr rule, VertexPtr expr) {
  switch (rule->type()) {
    case op_type_rule_func:
      apply_type_rule_func (rule, expr);
      break;
    case op_type_rule:
      apply_type_rule_type (rule, expr);
      break;
    case op_arg_ref:
      apply_arg_ref (rule, expr);
      break;
    case op_index:
      apply_index (rule, expr);
      break;
    default:
      kphp_error (0, "error in type rule");
      recalc_ptype <tp_Error>();
      break;
  }
}

void ExprNodeRecalc::recalc_func_call (VertexAdaptor <op_func_call> call) {
  FunctionPtr function = call->get_func_id();
  if (function->root->type_rule.not_null()) {
    apply_type_rule (function->root->type_rule.as <meta_op_type_rule>()->expr(), call);
  } else {
    set_lca (function, -1);
  }
}

void ExprNodeRecalc::recalc_var (VertexAdaptor <op_var> var) {
  set_lca (var->get_var_id());
}

void ExprNodeRecalc::recalc_push_back_return (VertexAdaptor <op_push_back_return> pb) {
  set_lca (pb->array(), &MultiKey::any_key (1));
}

void ExprNodeRecalc::recalc_index (VertexAdaptor <op_index> index) {
  set_lca (index->array(), &MultiKey::any_key (1));
}

void ExprNodeRecalc::recalc_set (VertexAdaptor <op_set> set) {
  set_lca (set->lhs());
}

void ExprNodeRecalc::recalc_double_arrow (VertexAdaptor <op_double_arrow> arrow) {
  set_lca (arrow->value());
}

void ExprNodeRecalc::recalc_foreach_param (VertexAdaptor <op_foreach_param> param) {
  set_lca (param->xs(), &MultiKey::any_key (1));
}

void ExprNodeRecalc::recalc_conv_array (VertexAdaptor <meta_op_unary_op> conv) {
  VertexPtr arg = conv->expr();
  //FIXME: (extra dependenty)
  add_dependency (as_rvalue (arg));
  if (fast_get_type (arg)->ptype() == tp_array) {
    set_lca (drop_or_false (as_rvalue (arg)));
  } else {
    recalc_ptype <tp_array>();
    if (fast_get_type (arg)->ptype() != tp_Unknown) { //hack
      set_lca_at (&MultiKey::any_key (1), tp_var);
    }
  }
}

void ExprNodeRecalc::recalc_min_max (VertexAdaptor <meta_op_builtin_func> func) {
  VertexRange args = func->args();
  if (args.size() == 1) {
    set_lca (args[0], &MultiKey::any_key (1));
  } else {
    FOREACH_VERTEX (args, i) {
      set_lca (*i);
    }
  }
}

void ExprNodeRecalc::recalc_array (VertexAdaptor <op_array> array) {
  recalc_ptype <tp_array>();
  FOREACH_VERTEX (array->args(), i) {
    set_lca_at (&MultiKey::any_key (1), *i);
  }
}

void ExprNodeRecalc::recalc_plus_minus (VertexAdaptor <meta_op_unary_op> expr) {
  set_lca (drop_or_false (as_rvalue (expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype <tp_var>();
  }
}

void ExprNodeRecalc::recalc_inc_dec (VertexAdaptor <meta_op_unary_op> expr) {
  //or false ???
  set_lca (drop_or_false (as_rvalue (expr->expr())));
}

void ExprNodeRecalc::recalc_noerr (VertexAdaptor <op_noerr> expr) {
  set_lca (as_rvalue (expr->expr()));
}


void ExprNodeRecalc::recalc_arithm (VertexAdaptor <meta_op_binary_op> expr) {
  VertexPtr lhs = expr->lhs();
  VertexPtr rhs = expr->rhs();

  //FIXME: (extra dependency)
  add_dependency (as_rvalue (lhs));
  add_dependency (as_rvalue (rhs));

  if (fast_get_type (lhs)->ptype() == tp_bool) {
    recalc_ptype <tp_int>();
  } else {
    set_lca (drop_or_false (as_rvalue (lhs)));
  }

  if (fast_get_type (rhs)->ptype() == tp_bool) {
    recalc_ptype <tp_int>();
  } else {
    set_lca (drop_or_false (as_rvalue (rhs)));
  }

  if (new_type()->ptype() == tp_string) {
    recalc_ptype <tp_var>();
  }
}

void ExprNodeRecalc::recalc_define_val (VertexAdaptor <op_define_val> define_val) {
  //TODO: fix?
  set_lca (define_val->get_define_id()->val);
}

void ExprNodeRecalc::recalc_expr (VertexPtr expr) {
  switch (expr->type()) {
    case op_require:
      recalc_require (expr);
      break;
    case op_ternary:
      recalc_ternary (expr);
      break;
    case op_func_call:
      recalc_func_call (expr);
      break;
    case op_common_type_rule:
    case op_gt_type_rule:
    case op_lt_type_rule:
    case op_eq_type_rule:
      apply_type_rule (expr.as <meta_op_type_rule>()->expr(), VertexPtr());
      break;
    case op_var:
      recalc_var (expr);
      break;
    case op_push_back_return:
      recalc_push_back_return (expr);
      break;
    case op_index:
      recalc_index (expr);
      break;
    case op_set:
      recalc_set (expr);
      break;
    case op_false:
      recalc_ptype <tp_False>();
      break;
    case op_log_or_let:
    case op_log_and_let:
    case op_log_xor_let:
    case op_log_or:
    case op_log_and:
    case op_log_not:
    case op_conv_bool:
    case op_true:
    case op_eq2:
    case op_eq3:
    case op_neq2:
    case op_neq3:
    case op_lt:
    case op_gt:
    case op_le:
    case op_ge:
    case op_isset:
    case op_exit:
      recalc_ptype <tp_bool>();
      break;

    case op_conv_string:
    case op_concat:
    case op_string_build:
    case op_string:
      recalc_ptype <tp_string>();
      break;

    case op_conv_int:
    case op_conv_int_l:
    case op_int_const:
    case op_mod:
    case op_not:
    case op_or:
    case op_and:
    case op_fork:
      recalc_ptype <tp_int>();
      break;

    case op_conv_float:
    case op_float_const:
    case op_div:
      recalc_ptype <tp_float>();
      break;

    case op_conv_uint:
      recalc_ptype <tp_UInt>();
      break;

    case op_conv_long:
      recalc_ptype <tp_Long>();
      break;

    case op_conv_ulong:
      recalc_ptype <tp_ULong>();
      break;

    case op_conv_regexp:
      recalc_ptype <tp_regexp>();
      break;

    case op_double_arrow:
      recalc_double_arrow (expr);
      break;

    case op_foreach_param:
      recalc_foreach_param (expr);
      break;

    case op_conv_array:
    case op_conv_array_l:
      recalc_conv_array (expr);
      break;

    case op_min:
    case op_max:
      recalc_min_max (expr);
      break;

    case op_array:
      recalc_array (expr);
      break;

    case op_conv_var:
    case op_null:
      recalc_ptype <tp_var>();
      break;

    case op_plus:
    case op_minus:
      recalc_plus_minus (expr);
      break;

    case op_prefix_inc:
    case op_prefix_dec:
    case op_postfix_inc:
    case op_postfix_dec:
      recalc_inc_dec (expr);
      break;

    case op_noerr:
      recalc_noerr (expr);
      break;
    
    case op_sub:
    case op_add:
    case op_mul:
    case op_shl:
    case op_shr:
      recalc_arithm (expr);
      break;

    case op_define_val:
      recalc_define_val (expr);
      break;

    default:
      recalc_ptype <tp_var>();
      break;
  }
}

bool ExprNodeRecalc::auto_edge_flag() {
  return node_->get_recalc_cnt() == 0;
}
ExprNodeRecalc::ExprNodeRecalc (tinf::ExprNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc (node, inferer) {
  }
tinf::ExprNode *ExprNodeRecalc::get_node() {
  return (tinf::ExprNode *)node_;
}
void ExprNodeRecalc::do_recalc() {
  tinf::ExprNode *node = get_node();
  VertexPtr expr = node->get_expr();
  //fprintf (stderr, "recalc expr %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());
  stage::set_location (expr->get_location());
  recalc_expr (expr);
}

const TypeData *tinf::get_type (VertexPtr vertex) {
  return get_type (vertex, tinf::get_inferer());
}
const TypeData *tinf::get_type (VarPtr var) {
  return get_type (var, tinf::get_inferer());
}
const TypeData *tinf::get_type (FunctionPtr function, int id) {
  return get_type (function, id, tinf::get_inferer());
}

