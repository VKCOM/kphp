#include "compiler/inferring/restriction-isset.h"

#include <sstream>

#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/expr-node.h"
#include "compiler/inferring/ifi.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-node.h"
#include "compiler/inferring/var-node.h"
#include "compiler/vertex.h"

RestrictionIsset::RestrictionIsset(tinf::Node *a) :
  a_(a) {
  //empty
}

const char *RestrictionIsset::get_description() {
  return desc.c_str();
}


static string remove_after_tab(const string &s) {
  return s.substr(0, s.find('\t'));
}

void RestrictionIsset::find_dangerous_isset_warning(const vector<tinf::Node *> &bt, tinf::Node *node, const string &msg __attribute__((unused))) {
  std::stringstream ss;
  ss << "isset, !==, ===, is_array or similar function result may differ from PHP\n" <<
     " Probably, this happened because " << remove_after_tab(node->get_description()) << " of type " <<
     type_out(node->get_type()) << " can't be null in KPHP, while it can be in PHP\n" <<
     " Chain of assignments:\n";

  for (auto const n : bt) {
    ss << "  " << n->get_description() << "\n";
  }
  ss << "  " << node->get_description() << "\n";
  desc = ss.str();
}

bool RestrictionIsset::isset_is_dangerous(int isset_flags, const TypeData *tp) {
  if (tp->use_optional()) {
    return false;
  }

  const PrimitiveType ptp = tp->get_real_ptype();
  if (isset_flags & ifi_isset) {
    return ptp != tp_var;
  }

  int check_mask = ifi_is_null;

  switch (ptp) {
    case tp_array: {
      check_mask |= ifi_is_array;
      break;
    }
    case tp_bool: {
      check_mask |= ifi_is_scalar | ifi_is_bool | ifi_is_false;
      break;
    }
    case tp_int: {
      check_mask |= ifi_is_scalar | ifi_is_numeric | ifi_is_integer | ifi_is_long;
      break;
    }
    case tp_float: {
      check_mask |= ifi_is_scalar | ifi_is_numeric | ifi_is_float;
      break;
    }
    case tp_string: {
      check_mask |= ifi_is_scalar | ifi_is_string;
      break;
    }
    default: {
      check_mask = 0;
      break;
    }
  }
  return (isset_flags & check_mask) != 0;
}

bool RestrictionIsset::find_dangerous_isset_dfs(int isset_flags, tinf::Node *node,
                                                vector<tinf::Node *> *bt) {
  if ((node->isset_was & isset_flags) == isset_flags) {
    return false;
  }
  node->isset_was |= isset_flags;

  tinf::TypeNode *type_node = dynamic_cast <tinf::TypeNode *> (node);
  if (type_node != nullptr) {
    return false;
  }

  tinf::ExprNode *expr_node = dynamic_cast <tinf::ExprNode *> (node);
  if (expr_node != nullptr) {
    VertexPtr v = expr_node->get_expr();
    if (v->type() == op_index && !vk::any_of_equal(tinf::get_type(v.as<op_index>()->array())->ptype(), tp_tuple, tp_shape) && isset_is_dangerous(isset_flags, node->get_type())) {
      node->isset_was = -1;
      find_dangerous_isset_warning(*bt, node, "[index]");
      return true;
    }
    if (auto var = v.try_as<op_var>()) {
      VarPtr from_var = var->var_id;
      if (from_var && from_var->get_uninited_flag() && isset_is_dangerous(isset_flags, node->get_type())) {
        node->isset_was = -1;
        find_dangerous_isset_warning(*bt, node, "[uninited varialbe]");
        return true;
      }

      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, tinf::get_tinf_node(from_var), bt)) {
        return true;
      }
      bt->pop_back();
    }
    if (auto call = v.try_as<op_func_call>()) {
      FunctionPtr func = call->func_id;
      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, tinf::get_tinf_node(func, -1), bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }

  tinf::VarNode *var_node = dynamic_cast <tinf::VarNode *> (node);
  if (var_node != nullptr) {
    VarPtr from_var = var_node->get_var();
    for (auto e : var_node->get_next()) {
      if (e->from_at->begin() != e->from_at->end()) {
        continue;
      }

      tinf::Node *to_node = e->to;

      /*** function f(&$a){}; f($b) fix ***/
      if (from_var) {
        VarPtr to_var;
        tinf::VarNode *to_var_node = dynamic_cast <tinf::VarNode *> (to_node);
        if (to_var_node != nullptr) {
          to_var = to_var_node->get_var();
        }
        if (to_var && to_var->type() == VarData::var_param_t &&
            !(to_var->holder_func == from_var->holder_func)) {
          continue;
        }
      }

      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, to_node, bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }
  return false;
}

bool RestrictionIsset::check_broken_restriction_impl() {
  vector<tinf::Node *> bt;
  return find_dangerous_isset_dfs(a_->isset_flags, a_, &bt);
}
