// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/cfg.h"

#include <stack>

#include "compiler/data/function-data.h"
#include "compiler/function-pass.h"
#include "compiler/debug.h"
#include "compiler/gentree.h"
#include "compiler/inferring/ifi.h"
#include "compiler/utils/dsu.h"
#include "compiler/utils/idgen.h"
#include "compiler/utils/string-utils.h"

namespace cfg {
// just simple int id type
class Node {
  DEBUG_STRING_METHOD { return std::to_string(id_); }

public:
  explicit operator bool() const { return id_ != -1; }
  void clear() { id_ = -1; }

  friend int get_index(const Node &i) { return i.id_; }
  friend void set_index(Node &i, int index) { i.id_ = index; }

private:
  int id_{-1};
};

enum UsageType : uint8_t {
  usage_write_t,
  usage_read_t,
  usage_weak_write_t,
  usage_type_hint_t,
  usage_type_check_t,
};

struct UsageData {
  DEBUG_STRING_METHOD { return std::to_string(id); }

  int id = -1;
  int part_id = -1;
  Node node;
  UsageType type;
  is_func_id_t checked_type;

  VertexAdaptor<op_var> v;

  UsageData(UsageType type, VertexAdaptor<op_var> v) :
    type(type),
    v(v) {
  }
};

using UsagePtr = Id<UsageData>;

struct SubTreeData {
  VertexPtr v;
  bool recursive_flag;
};

struct VertexUsage {
  bool used = false;
  bool used_rec = false;
};

struct VarSplitData {
  int n = 0;

  IdGen<UsagePtr> usage_gen;
  IdMap<UsagePtr> parent;       // dsu

  VarSplitData() {
    usage_gen.add_id_map(&parent);
  }
};

using VarSplitPtr = Id<VarSplitData>;

class CFG {
  CFGData data;
  IdGen<Node> node_gen;
  IdMap<std::vector<Node>> node_next, node_prev;
  IdMap<std::vector<UsagePtr>> node_usages;
  IdMap<std::vector<SubTreeData>> node_subtrees;
  IdMap<VertexUsage> vertex_usage;
  IdMap<is_func_id_t> vertex_convertions;
  int cur_dfs_mark = 0;
  Node current_start;

  IdMap<int> node_was;
  IdMap<is_func_id_t> node_checked_type;
  IdMap<UsagePtr> node_mark_dfs;
  IdMap<int> node_mark_dfs_type_hint;
  IdMap<VarSplitPtr> var_split_data;

  std::vector<std::vector<Node>> continue_nodes;
  std::vector<std::vector<Node>> break_nodes;
  void create_cfg_enter_cycle();
  void create_cfg_exit_cycle(Node continue_dest, Node break_dest);
  void create_cfg_add_break_node(Node v, int depth);
  void create_cfg_add_continue_node(Node v, int depth);

  std::vector<std::vector<Node>> exception_nodes;
  void create_cfg_begin_try();
  void create_cfg_end_try();
  void create_cfg_register_exception(Node from);

  Node new_node();
  UsagePtr new_usage(UsageType type, VertexAdaptor<op_var> v);
  void add_usage(Node node, UsagePtr usage);
  void add_subtree(Node node, VertexPtr subtree_vertex, bool recursive_flag);
  void add_edge(Node from, Node to);
  void collect_ref_vars(VertexPtr v, std::unordered_set<VarPtr> &ref);
  std::vector<VarPtr> collect_splittable_vars(FunctionPtr func);
  void create_cfg(VertexPtr tree_node, Node *res_start, Node *res_finish, bool write_flag = false, bool weak_write_flag = false);
  void create_condition_cfg(VertexPtr tree_node, Node *res_start, Node *res_true, Node *res_false);

  void calc_used(Node v);
  void confirm_usage(VertexPtr, bool recursive_flags);

  void dfs_checked_types(Node v, VarPtr var, is_func_id_t current_mask);

  bool try_uni_usages(UsagePtr usage, UsagePtr another_usage);
  void dfs_uni_rw_usages(Node v, UsagePtr usage);
  void dfs_apply_type_hint(Node v, UsagePtr usage);
  void process_var(FunctionPtr function, VarPtr v);
  int register_vertices(VertexPtr v, int N);
  void add_uninited_var(VertexAdaptor<op_var> v);
  void split_var(FunctionPtr function, VarPtr var, std::vector<std::vector<VertexAdaptor<op_var>>> &parts);
public:
  void process_function(FunctionPtr func);
  CFGData move_data() {
    return std::move(data);
  }
};

Node CFG::new_node() {
  Node res;
  node_gen.init_id(&res);
  return res;
}

UsagePtr CFG::new_usage(UsageType type, VertexAdaptor<op_var> v) {
  VarPtr var = v->var_id;
  kphp_assert (var);
  if (get_index(var) < 0) {    // non-splittable var
    return {};
  }
  VarSplitPtr var_split = var_split_data[var];
  kphp_assert(var_split);
  UsagePtr res = UsagePtr(new UsageData(type, v));
  var_split->usage_gen.init_id(&res);
  var_split->parent[res] = res;
  return res;
}

void CFG::add_usage(Node node, UsagePtr usage) {
  if (!usage) {
    return;
  }
  //fprintf(stderr, "%s is used at node %d with type %d\n", usage->v->get_string().c_str(), get_index(node), usage->type);
  //hope that one node will contain usages of the same type
  kphp_assert (node_usages[node].empty() || node_usages[node].back()->type == usage->type);
  node_usages[node].push_back(usage);
  usage->node = node;

//    VertexPtr v = usage->v; //TODO assigned but not used
}

void CFG::add_subtree(Node node, VertexPtr subtree_vertex, bool recursive_flag) {
  kphp_assert (node && subtree_vertex);
  node_subtrees[node].emplace_back(SubTreeData{subtree_vertex, recursive_flag});
}

void CFG::add_edge(Node from, Node to) {
  if (from && to) {
    //fprintf(stderr, "%s, add-edge: %d->%d\n", stage::get_function_name().c_str(), get_index(from), get_index(to));
    node_next[from].push_back(to);
    node_prev[to].push_back(from);
  }
}

void CFG::collect_ref_vars(VertexPtr v, std::unordered_set<VarPtr> &ref) {
  if (v->type() == op_var && v->ref_flag) {
    ref.emplace(v.as<op_var>()->var_id);
  }
  for (auto i : *v) {
    collect_ref_vars(i, ref);
  }
}

std::vector<VarPtr> CFG::collect_splittable_vars(FunctionPtr func) {
  std::unordered_set<VarPtr> ref_vars;
  collect_ref_vars(func->root, ref_vars);

  auto params = func->get_params();
  std::vector<VarPtr> splittable_vars;
  splittable_vars.reserve(func->local_var_ids.size() + params.size());
  for (auto var : func->local_var_ids) {
    if (ref_vars.find(var) == ref_vars.end()) {
      splittable_vars.emplace_back(var);
    }
  }

  for (VarPtr var : func->param_ids) {
    auto param = params[var->param_i].as<op_func_param>();
    VertexPtr init = param->var();
    kphp_assert (init->type() == op_var);
    if (!init->ref_flag) {
      splittable_vars.emplace_back(var);
    }
  }

  return splittable_vars;
}

void CFG::create_cfg_enter_cycle() {
  continue_nodes.resize(continue_nodes.size() + 1);
  break_nodes.resize(break_nodes.size() + 1);
}

void CFG::create_cfg_exit_cycle(Node continue_dest, Node break_dest) {
  for (Node i : continue_nodes.back()) {
    add_edge(i, continue_dest);
  }
  for (Node i : break_nodes.back()) {
    add_edge(i, break_dest);
  }
  continue_nodes.pop_back();
  break_nodes.pop_back();
}

void CFG::create_cfg_add_break_node(Node v, int depth) {
  kphp_assert (depth >= 1);
  int i = (int)break_nodes.size() - depth;
  kphp_assert (i >= 0);
  break_nodes[i].push_back(v);
}

void CFG::create_cfg_add_continue_node(Node v, int depth) {
  kphp_assert (depth >= 1);
  int i = (int)continue_nodes.size() - depth;
  kphp_assert (i >= 0);
  continue_nodes[i].push_back(v);
}

void CFG::create_cfg_begin_try() {
  exception_nodes.resize(exception_nodes.size() + 1);
}

void CFG::create_cfg_end_try() {
  exception_nodes.pop_back();
}

void CFG::create_cfg_register_exception(Node from) {
  if (!exception_nodes.empty()) {
    exception_nodes.back().push_back(from);
  }
}

void CFG::create_condition_cfg(VertexPtr tree_node, Node *res_start, Node *res_true, Node *res_false) {
  switch (tree_node->type()) {
    case op_conv_bool: {
      create_condition_cfg(tree_node.as<op_conv_bool>()->expr(), res_start, res_true, res_false);
      break;
    }
    case op_log_not: {
      create_condition_cfg(tree_node.as<op_log_not>()->expr(), res_start, res_false, res_true);
      break;
    }
    case op_log_and:
    case op_log_or: {
      Node first_start, first_true, first_false, second_start, second_true, second_false;
      auto op = tree_node.as<meta_op_binary>();
      create_condition_cfg(op->lhs(), &first_start, &first_true, &first_false);
      create_condition_cfg(op->rhs(), &second_start, &second_true, &second_false);
      *res_start = first_start;
      *res_true = new_node();
      *res_false = new_node();
      add_edge(first_true, tree_node->type() == op_log_and ? second_start : *res_true);
      add_edge(first_false, tree_node->type() == op_log_or ? second_start : *res_false);
      add_edge(second_true, *res_true);
      add_edge(second_false, *res_false);
      break;
    }
    default: {
      Node res_finish;
      create_cfg(tree_node, res_start, &res_finish);
      *res_true = new_node();
      *res_false = new_node();
      auto add_type_check_usage = [&](Node to, int type, VertexAdaptor<op_var> var) {
        if (vk::any_of_equal(var->var_id->type(), VarData::var_local_t, VarData::var_param_t) && !var->var_id->is_foreach_reference && !var->var_id->is_reference) {
          UsagePtr usage = new_usage(usage_type_check_t, var);
          usage->checked_type = static_cast<is_func_id_t>(type);
          add_usage(to, usage);
        }
      };
      if (auto call = tree_node.try_as<op_func_call>()) {
        is_func_id_t type = get_ifi_id(tree_node);
        if (type != ifi_error) {
          auto var = call->args()[0].try_as<op_var>();
          if (auto mixed_conv = call->args()[0].try_as<op_conv_mixed>()) {
            // we're going to analyze internal var, instead of fake cast to mixed
            var = mixed_conv->expr().try_as<op_var>();
          }
          if (var) {
            is_func_id_t true_type{};
            if (type == ifi_is_bool) {
              true_type = static_cast<is_func_id_t>(ifi_is_bool | ifi_is_false);
            } else if (type == ifi_is_scalar) {
              true_type = static_cast<is_func_id_t>(ifi_is_false | ifi_is_bool | ifi_is_integer | ifi_is_float | ifi_is_string);
            } else if (type == ifi_is_numeric) {
              true_type = static_cast<is_func_id_t>(ifi_is_integer | ifi_is_float | ifi_is_string);
            } else {
              true_type = type;
            }
            is_func_id_t false_type{};
            if (type == ifi_is_numeric) {
              false_type = static_cast<is_func_id_t>((ifi_any_type & ~true_type) | ifi_is_string);
            } else {
              false_type = static_cast<is_func_id_t>(ifi_any_type & ~true_type);
            }
            add_type_check_usage(*res_true, true_type, var);
            add_type_check_usage(*res_false, false_type, var);
          }
        }
      } else if (auto isset = tree_node.try_as<op_isset>()) {
        if (auto var = isset->expr().try_as<op_var>()) {
          add_type_check_usage(*res_true, ifi_any_type & ~ifi_is_null, var);
          add_type_check_usage(*res_false, ifi_is_null, var);
        }
      } else if (auto var = tree_node.try_as<op_var>()) {
        add_type_check_usage(*res_true, ifi_any_type & ~(ifi_is_false | ifi_is_null), var);
      } else if (tree_node->type() == op_eq3) {
        if (auto var = tree_node.try_as<meta_op_binary>()->lhs().try_as<op_var>()) {
          if (tree_node.try_as<meta_op_binary>()->rhs()->type() == op_false) {
            add_type_check_usage(*res_true, ifi_is_false, var);
            add_type_check_usage(*res_false, ifi_any_type & ~ifi_is_false, var);
          }
        }
      } else if (tree_node->type() == op_eq2) {
        if (auto var = tree_node.try_as<meta_op_binary>()->lhs().try_as<op_var>()) {
          if (vk::any_of_equal(tree_node.try_as<meta_op_binary>()->rhs()->type(), op_false, op_null)) {
            add_type_check_usage(*res_false, ifi_any_type & ~(ifi_is_false|ifi_is_null), var);
          }
        }
      }
      add_edge(res_finish, *res_true);
      add_edge(res_finish, *res_false);
      break;
    }
  }

  add_subtree(*res_start, tree_node, false);
}

void CFG::create_cfg(VertexPtr tree_node, Node *res_start, Node *res_finish, bool write_flag, bool weak_write_flag) {
  stage::set_location(tree_node->location);
  bool recursive_flag = false;
  switch (tree_node->type()) {
    // vararg operators
    case op_array:
    case op_tuple:
    case op_shape:
    case op_seq_comma:
    case op_seq_rval:
    case op_seq:
    case op_string_build: {
      Node a, b, end;
      if (tree_node->empty()) {
        *res_start = *res_finish = new_node();
        break;
      }
      VertexRange args = tree_node.as<meta_op_varg>()->args();
      create_cfg(args[0], res_start, &b);
      end = b;
      for (int i = 1; i < tree_node->size(); i++) {
        create_cfg(args[i], &a, &b);
        add_edge(end, a);
        end = b;
      }
      *res_finish = end;
      break;
    }

    // simple just-value operators
    case op_int_const:
    case op_float_const:
    case op_true:
    case op_false:
    case op_null:
    case op_empty:
    case op_alloc: {
      *res_start = *res_finish = new_node();
      break;
    }

    // simple binary operators
    case op_add:
    case op_sub:
    case op_mul:
    case op_div:
    case op_shl:
    case op_shr:
    case op_concat:
    case op_le:
    case op_lt:
    case op_null_coalesce:
    case op_instanceof:
    case op_and:
    case op_xor:
    case op_or:
    case op_eq3:
    case op_eq2:
    case op_mod:
    case op_pow:
    case op_set_add:
    case op_set_sub:
    case op_set_mul:
    case op_set_div:
    case op_set_mod:
    case op_set_pow:
    case op_set_and:
    case op_set_or:
    case op_set_xor:
    case op_set_dot:
    case op_set_shr:
    case op_set_shl:
    case op_spaceship:
    case op_double_arrow: {
      auto op = tree_node.as<meta_op_binary>();
      Node first_finish, second_start;
      create_cfg(op->lhs(), res_start, &first_finish);
      create_cfg(op->rhs(), &second_start, res_finish);
      add_edge(first_finish, second_start);
      break;
    }

    // simple unary operators
    case op_conv_int:
    case op_conv_int_l:
    case op_conv_float:
    case op_conv_string:
    case op_conv_string_l:
    case op_conv_array:
    case op_conv_array_l:
    case op_conv_object:
    case op_conv_mixed:
    case op_force_mixed:
    case op_conv_regexp:
    case op_conv_bool:
    case op_conv_drop_null:
    case op_conv_drop_false:
    case op_isset:
    case op_unset:
    case op_postfix_inc:
    case op_postfix_dec:
    case op_prefix_inc:
    case op_prefix_dec:
    case op_plus:
    case op_minus:
    case op_move:
    case op_log_not:
    case op_noerr:
    case op_not:
    case op_addr:
    case op_clone: {
      create_cfg(tree_node.as<meta_op_unary>()->expr(), res_start, res_finish);
      break;
    }

    case op_instance_prop: {
      create_cfg(tree_node.as<op_instance_prop>()->instance(), res_start, res_finish);
      break;
    }
    case op_index: {
      Node var_start, var_finish;
      auto index = tree_node.as<op_index>();
      create_cfg(index->array(), &var_start, &var_finish, false, write_flag || weak_write_flag);
      Node start = var_start;
      Node finish = var_finish;
      if (index->has_key()) {
        Node index_start, index_finish;
        create_cfg(index->key(), &index_start, &index_finish);
        add_edge(index_finish, start);
        start = index_start;
      }
      *res_start = start;
      *res_finish = finish;
      break;
    }
    case op_log_and:
    case op_log_or:
    case op_log_and_let:
    case op_log_or_let:
    case op_log_xor_let: {
      Node first_start, first_finish, second_start, second_finish;
      auto op = tree_node.as<meta_op_binary>();
      create_cfg(op->lhs(), &first_start, &first_finish);
      create_cfg(op->rhs(), &second_start, &second_finish);
      Node finish = new_node();
      add_edge(first_finish, second_start);
      add_edge(second_finish, finish);
      add_edge(first_finish, finish);
      *res_start = first_start;
      *res_finish = finish;
      break;
    }
    case op_func_call: {
      FunctionPtr func = tree_node.as<op_func_call>()->func_id;

      Node start, a, b;
      start = new_node();
      *res_start = start;

      int ii = 0;
      for (auto cur : tree_node.as<op_func_call>()->args()) {
        bool new_weak_write_flag = false;

        if (func) {
          auto param = func->get_params()[ii].try_as<op_func_param>();
          if (param && param->var()->ref_flag) {
            new_weak_write_flag = true;
          }
        }

        kphp_assert (cur);
        create_cfg(cur, &a, &b, false, new_weak_write_flag);
        add_edge(start, a);
        start = b;

        ii++;
      }
      if (func->is_no_return) {
        *res_finish = Node();
      } else {
        *res_finish = start;
      }

      if (func->can_throw()) {
        create_cfg_register_exception(*res_finish);
      }
      break;
    }
    case op_exception_constructor_call: {
      auto op = tree_node.as<op_exception_constructor_call>();
      Node call_end, file_start, file_end, line_start;
      create_cfg(op->constructor_call(), res_start, &call_end);
      create_cfg(op->file_arg(), &file_start, &file_end);
      create_cfg(op->line_arg(), &line_start, res_finish);
      add_edge(call_end, file_start);
      add_edge(file_end, line_start);
      break;
    }
    case op_fork: {
      create_cfg(tree_node.as<op_fork>()->func_call(), res_start, res_finish);
      break;
    }
    case op_func_ptr: {
      int n = std::distance(tree_node.as<op_func_ptr>()->begin(), tree_node.as<op_func_ptr>()->end());
      kphp_assert(n == 0 || n == 1);    // empty if a string callback, op_func_call if a lambda
      if (n == 1) {
        create_cfg(*tree_node.as<op_func_ptr>()->begin(), res_start, res_finish);
      } else {
        *res_start = *res_finish = new_node();
      }
      break;
    }
    case op_return: {
      auto return_op = tree_node.as<op_return>();
      if (return_op->has_expr()) {
        Node tmp;
        create_cfg(return_op->expr(), res_start, &tmp);
      } else {
        *res_start = new_node();
      }
      *res_finish = Node();
      break;
    }
    case op_set: {
      auto set_op = tree_node.as<op_set>();
      Node a, b;
      create_cfg(set_op->rhs(), res_start, &a);
      create_cfg(set_op->lhs(), &b, res_finish, true);
      add_edge(a, b);
      break;
    }
    case op_list: {
      auto list = tree_node.as<op_list>();
      Node prev;
      create_cfg(list->array(), res_start, &prev);
      for (auto param : list->list().get_reversed_range()) {
        Node a, b;
        create_cfg(param, &a, &b, true);
        add_edge(prev, a);
        prev = b;
      }
      *res_finish = prev;
      break;
    }
    case op_list_keyval: {
      const auto kv = tree_node.as<op_list_keyval>();
      Node a, b;
      create_cfg(kv->var(), res_start, &a, true);
      create_cfg(kv->key(), &b, res_finish);
      add_edge(a, b);
      break;
    }
    case op_var: {
      Node res = new_node();
      UsagePtr usage = new_usage(write_flag ? usage_write_t : weak_write_flag ? usage_weak_write_t : usage_read_t, tree_node.as<op_var>());
      add_usage(res, usage);
      *res_start = *res_finish = res;
      break;
    }
    case op_phpdoc_var: {
      Node res = new_node();
      UsagePtr usage = new_usage(usage_type_hint_t, tree_node.as<op_phpdoc_var>()->var());
      add_usage(res, usage);
      *res_start = *res_finish = res;
      add_subtree(*res_start, tree_node.as<op_phpdoc_var>()->var(), false);
      break;
    }
    case op_if: {
      auto if_op = tree_node.as<op_if>();
      Node finish = new_node();
      Node cond_true, cond_false, if_start, if_finish;
      create_condition_cfg(if_op->cond(), res_start, &cond_true, &cond_false);
      create_cfg(if_op->true_cmd(), &if_start, &if_finish);
      add_edge(cond_true, if_start);
      add_edge(if_finish, finish);
      if (if_op->has_false_cmd()) {
        Node else_start, else_finish;
        create_cfg(if_op->false_cmd(), &else_start, &else_finish);
        add_edge(cond_false, else_start);
        add_edge(else_finish, finish);
      } else {
        add_edge(cond_false, finish);
      }

      *res_finish = finish;
      break;
    }
    case op_ternary: {
      auto ternary_op = tree_node.as<op_ternary>();
      Node finish = new_node();
      Node cond_true, cond_false;

      Node if_start, if_finish;
      create_condition_cfg(ternary_op->cond(), res_start, &cond_true, &cond_false);
      create_cfg(ternary_op->true_expr(), &if_start, &if_finish);
      add_edge(cond_true, if_start);
      add_edge(if_finish, finish);

      Node else_start, else_finish;
      create_cfg(ternary_op->false_expr(), &else_start, &else_finish);
      add_edge(cond_false, else_start);
      add_edge(else_finish, finish);

      *res_finish = finish;
      break;
    }
    case op_break: {
      auto break_op = tree_node.as<op_break>();
      recursive_flag = true;
      Node start = new_node(), finish = Node();
      create_cfg_add_break_node(start, atoi(break_op->level()->get_string().c_str()));

      *res_start = start;
      *res_finish = finish;
      break;
    }
    case op_continue: {
      auto continue_op = tree_node.as<op_continue>();
      recursive_flag = true;
      Node start = new_node(), finish = Node();
      create_cfg_add_continue_node(start, atoi(continue_op->level()->get_string().c_str()));

      *res_start = start;
      *res_finish = finish;
      break;
    }
    case op_for: {
      create_cfg_enter_cycle();

      auto for_op = tree_node.as<op_for>();

      Node init_start, init_finish;
      create_cfg(for_op->pre_cond(), &init_start, &init_finish);

      Node cond_start, cond_finish_true, cond_finish_false;
      create_condition_cfg(for_op->cond(), &cond_start, &cond_finish_true, &cond_finish_false);

      Node inc_start, inc_finish;
      create_cfg(for_op->post_cond(), &inc_start, &inc_finish);

      Node action_start, action_finish_pre, action_finish = new_node();
      create_cfg(for_op->cmd(), &action_start, &action_finish_pre);
      add_edge(action_finish_pre, action_finish);

      add_edge(init_finish, cond_start);
      add_edge(cond_finish_true, action_start);
      add_edge(action_finish, inc_start);
      add_edge(inc_finish, cond_start);

      Node finish = new_node();
      add_edge(cond_finish_false, finish);

      *res_start = init_start;
      *res_finish = finish;

      create_cfg_exit_cycle(action_finish, finish);
      break;
    }
    case op_do:
    case op_while: {
      create_cfg_enter_cycle();

      VertexPtr cond, cmd;
      if (auto do_op = tree_node.try_as<op_do>()) {
        cond = do_op->cond();
        cmd = do_op->cmd();
      } else if (auto while_op = tree_node.try_as<op_while>()) {
        cond = while_op->cond();
        cmd = while_op->cmd();
      } else {
        kphp_fail();
      }


      Node cond_start, cond_finish_true, cond_finish_false;
      create_condition_cfg(cond, &cond_start, &cond_finish_true, &cond_finish_false);

      Node action_start, action_finish_pre, action_finish = new_node();
      create_cfg(cmd, &action_start, &action_finish_pre);
      add_edge(action_finish_pre, action_finish);

      add_edge(cond_finish_true, action_start);
      add_edge(action_finish, cond_start);

      Node finish = new_node();
      add_edge(cond_finish_false, finish);

      if (tree_node->type() == op_do) {
        *res_start = action_start;
      } else if (tree_node->type() == op_while) {
        *res_start = cond_start;
      } else {
        kphp_fail();
      }
      *res_finish = finish;

      if (tree_node->type() == op_do && action_finish_pre) {
        add_subtree(*res_start, cond, true);
      }

      create_cfg_exit_cycle(action_finish, finish);
      break;
    }
    case op_foreach: {
      create_cfg_enter_cycle();

      auto foreach_op = tree_node.as<op_foreach>();

      auto foreach_param = foreach_op->params();
      Node val_start, val_finish;

      create_cfg(foreach_param->xs(), &val_start, &val_finish);

      Node writes = new_node();
      add_usage(writes, new_usage(usage_write_t, foreach_param->x()));
      if (!foreach_param->x()->ref_flag) {
        add_usage(writes, new_usage(usage_write_t, foreach_param->temp_var().as<op_var>()));
      }
      if (foreach_param->has_key()) {
        add_usage(writes, new_usage(usage_write_t, foreach_param->key()));
      }

      //?? not sure
      add_subtree(val_start, foreach_param, true);

      Node finish = new_node();

      Node cond_start = val_start;
      Node cond_check = new_node();
      Node cond_true = writes;
      Node cond_false = finish;

      add_edge(val_finish, cond_check);
      add_edge(cond_check, cond_true);
      add_edge(cond_check, cond_false);

      Node action_start, action_finish_pre, action_finish = new_node();
      create_cfg(foreach_op->cmd(), &action_start, &action_finish_pre);
      add_edge(action_finish_pre, action_finish);

      add_edge(cond_true, action_start);
      add_edge(action_finish, cond_check);

      *res_start = cond_start;
      *res_finish = finish;

      create_cfg_exit_cycle(action_finish, finish);
      break;
    }
    case op_switch: {
      create_cfg_enter_cycle();

      auto switch_op = tree_node.as<op_switch>();
      Node cond_start;
      Node cond_finish;
      create_cfg(switch_op->condition(), &cond_start, &cond_finish);

      Node prev_finish;
      Node prev_var_finish = cond_finish;

      Node vars_init = new_node();
      Node vars_read = new_node();

      add_edge(vars_init, vars_read);
      auto add_usages_for_switch_var = [&](VertexAdaptor<op_var> switch_var) {
        add_usage(vars_init, new_usage(usage_write_t, switch_var));
        add_usage(vars_read, new_usage(usage_read_t, switch_var));
        add_subtree(vars_init, switch_var, false);
        add_subtree(vars_read, switch_var, false);
      };
      add_usages_for_switch_var(switch_op->condition_on_switch());
      add_usages_for_switch_var(switch_op->matched_with_one_case());

      bool was_default = false;
      Node default_start;
      for (auto i : switch_op->cases()) {
        VertexPtr expr, cmd;
        bool is_default = false;
        if (auto cs = i.try_as<op_case>()) {
          expr = cs->expr();
          cmd = cs->cmd();
        } else if (auto def = i.try_as<op_default>()) {
          is_default = true;
          cmd = def->cmd();
        } else {
          kphp_fail();
        }

        Node cur_start, cur_finish;
        create_cfg(cmd, &cur_start, &cur_finish);
        add_edge(prev_finish, cur_start);
        prev_finish = cur_finish;

        if (is_default) {
          default_start = cur_start;
          was_default = true;
        } else {
          Node cur_var_start, cur_var_finish;
          create_cfg(expr, &cur_var_start, &cur_var_finish);
          add_edge(cur_var_finish, cur_start);
          add_edge(prev_var_finish, cur_var_start);
          prev_var_finish = cur_var_finish;
        }

        add_subtree(cond_start, i, false);
      }
      Node finish = new_node();
      add_edge(prev_finish, finish);
      if (was_default) {
        add_edge(prev_var_finish, default_start);
      } else {
        add_edge(prev_var_finish, finish);
      }

      add_edge(vars_read, cond_start);
      *res_start = vars_init;
      *res_finish = finish;

      create_cfg_exit_cycle(finish, finish);
      break;
    }
    case op_throw: {
      auto throw_op = tree_node.as<op_throw>();
      Node throw_start, throw_finish;
      create_cfg(throw_op->exception(), &throw_start, &throw_finish);
      create_cfg_register_exception(throw_finish);

      *res_start = throw_start;
      //*res_finish = throw_finish;
      *res_finish = new_node();
      break;
    }
    case op_try: {
      auto try_op = tree_node.as<op_try>();

      Node try_start, try_finish;
      create_cfg_begin_try();
      create_cfg(try_op->try_cmd(), &try_start, &try_finish);
      std::vector<Node> exceptions = exception_nodes.back();
      create_cfg_end_try();

      Node finish = new_node();
      add_edge(try_finish, finish);

      Node catch_list_start = new_node();
      for (Node e : exceptions) {
        add_edge(e, catch_list_start);
      }

      // connect catch_list_start to every catch block
      for (auto c : try_op->catch_list()) {
        auto catch_op = c.as<op_catch>();
        Node exception_start, exception_finish;
        create_cfg(catch_op->var(), &exception_start, &exception_finish, true);

        add_edge(catch_list_start, exception_start);

        Node catch_start, catch_finish;
        create_cfg(catch_op->cmd(), &catch_start, &catch_finish);

        add_edge(exception_finish, catch_start);
        add_edge(catch_finish, finish);

        add_subtree(catch_list_start, catch_op, false);
        add_subtree(exception_start, catch_op->var(), false);
        add_subtree(catch_start, catch_op->cmd(), true);
      }

      if (!try_op->catches_all) {
        // propagate all thrown exceptions
        for (Node n : exceptions) {
          create_cfg_register_exception(n);
        }
      }

      *res_start = try_start;
      *res_finish = finish;
      break;
    }

    case op_function: {
      auto function = tree_node.as<op_function>();
      Node a, b;
      create_cfg(function->param_list(), res_start, &a);
      create_cfg(function->cmd(), &b, res_finish);
      add_edge(a, b);
      break;
    }
    case op_func_param_list: {
      auto param_list = tree_node.as<op_func_param_list>();
      Node a = new_node();
      for (auto p : param_list->params()) {
        add_usage(a, new_usage(usage_read_t, p.as<op_func_param>()->var()));
      }
      *res_start = a;
      *res_finish = a;
      recursive_flag = true;
      break;
    }

    default: {
      kphp_error(0, fmt_format("cfg doesnt handle this type of operation: {}", static_cast<int>(tree_node->type())));
      kphp_fail();
      return;
    }
  }

  add_subtree(*res_start, tree_node, recursive_flag);
}

bool CFG::try_uni_usages(UsagePtr usage, UsagePtr another_usage) {
  VarPtr var = usage->v->var_id;
  VarPtr another_var = another_usage->v->var_id;
  if (var == another_var) {
    VarSplitPtr var_split = var_split_data[var];
    kphp_assert (var_split);
    dsu_uni(&var_split->parent, usage, another_usage);
    return true;
  }
  return false;
}

void CFG::dfs_uni_rw_usages(Node v, UsagePtr usage) {
  UsagePtr other_usage = node_mark_dfs[v];
  if (other_usage) {
    try_uni_usages(usage, other_usage);
    return;
  }
  node_mark_dfs[v] = usage;

  bool write_usage_found = false;
  for (auto another_usage : node_usages[v]) {
    if (try_uni_usages(usage, another_usage) && another_usage->type == usage_write_t) {
      write_usage_found = true;
    }
  }
  if (write_usage_found) {
    return;
  }
  for (Node i : node_prev[v]) {
    dfs_uni_rw_usages(i, usage);
  }
}

void CFG::dfs_apply_type_hint(Node v, UsagePtr usage) {
  UsagePtr other_usage = node_mark_dfs[v];
  if (other_usage && other_usage->type != usage_type_hint_t) {
    try_uni_usages(usage, other_usage);
  }
  // this check is needed to avoid the infinite loop (node_next is a graph with cycles)
  if (node_mark_dfs_type_hint[v]) {
    return;
  }
  node_mark_dfs[v] = usage;
  node_mark_dfs_type_hint[v] = 1;

  bool another_type_hint_found = false;
  for (UsagePtr another_usage : node_usages[v]) {
    if (another_usage->type == usage_type_hint_t && get_index(usage) != get_index(another_usage)) {
      another_type_hint_found = true;
    } else {
      try_uni_usages(usage, another_usage);
    }
  }
  if (another_type_hint_found) {
    return;
  }
  for (Node i : node_next[v]) {
    dfs_apply_type_hint(i, usage);
  }
}

void CFG::dfs_checked_types(Node v, VarPtr var, is_func_id_t current_mask) {
  if ((node_checked_type[v] | current_mask) == node_checked_type[v]) {
    return;
  }
  node_checked_type[v] = static_cast<is_func_id_t>(node_checked_type[v] | current_mask);

  for (UsagePtr another_usage : node_usages[v]) {
    if (another_usage->v->var_id == var) {
      if (another_usage->type == usage_write_t || another_usage->type == usage_weak_write_t) {
        current_mask = ifi_any_type;
      } else if (another_usage->type == usage_type_check_t) {
        current_mask = static_cast<is_func_id_t>(current_mask & (another_usage->checked_type | ifi_unset));
      }
    }
  }

  for (Node i : node_next[v]) {
    dfs_checked_types(i, var, current_mask);
  }
}

void CFG::add_uninited_var(VertexAdaptor<op_var> v) {
  if (v && v->extra_type != op_ex_var_superlocal && v->extra_type != op_ex_var_this) {
    data.uninited_vars.push_back(v);
    v->var_id->set_uninited_flag(true);
  }
}

void CFG::split_var(FunctionPtr function, VarPtr var, vector<std::vector<VertexAdaptor<op_var>>> &parts) {
  kphp_assert(var->type() == VarData::var_local_t || var->type() == VarData::var_param_t);
  if (parts.empty()) {
    if (var->type() == VarData::var_local_t) {
      function->local_var_ids.erase(
        std::find(
          function->local_var_ids.begin(),
          function->local_var_ids.end(),
          var));
    }
    return;
  }
  kphp_assert(parts.size() > 1);

  Assumption a = assumption_get_for_var(function, var->name);

  for (size_t i = 0; i < parts.size(); i++) {
    // name$v1, name$v2 and so on, but name (0th copy) is kept as is
    const std::string &new_name = i ? var->name + "$v" + std::to_string(i) : var->name;
    VarPtr new_var = G->create_var(new_name, var->type());
    new_var->holder_func = var->holder_func;

    if (i && a.has_instance()) {
      assumption_add_for_var(function, new_name, a);
    }

    for (auto v : parts[i]) {
      v->var_id = new_var;
    }

    VertexRange params = function->get_params();
    if (var->type() == VarData::var_local_t) {
      new_var->type() = VarData::var_local_t;
      function->local_var_ids.push_back(new_var);
    } else if (var->type() == VarData::var_param_t) {
      bool was_var = std::find(
        parts[i].begin(),
        parts[i].end(),
        params[var->param_i].as<op_func_param>()->var()
      ) != parts[i].end();

      if (was_var) { //union of part that contains function argument
        new_var->type() = VarData::var_param_t;
        new_var->param_i = var->param_i;
        new_var->init_val = var->init_val;
        new_var->is_read_only = var->is_read_only;
        new_var->tinf_node.init_as_argument(new_var);
        function->param_ids[var->param_i] = new_var;
      } else {
        new_var->type() = VarData::var_local_t;
        function->local_var_ids.push_back(new_var);
      }
    } else {
      kphp_fail();
    }

  }

  if (var->type() == VarData::var_local_t) {
    auto tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
    if (function->local_var_ids.end() != tmp) {
      function->local_var_ids.erase(tmp);
    } else {
      kphp_fail();
    }
  }

  data.todo_parts.emplace_back(std::move(parts));
}

void CFG::process_var(FunctionPtr function, VarPtr var) {
  VarSplitPtr var_split = var_split_data[var];
  kphp_assert (var_split);

  cur_dfs_mark++;
  std::fill(node_checked_type.begin(), node_checked_type.end(), static_cast<is_func_id_t>(0));
  dfs_checked_types(current_start, var, static_cast<is_func_id_t>(ifi_any_type | ((var->type() == VarData::var_param_t) ? 0 : ifi_unset)));
  for (UsagePtr u : var_split->usage_gen) {
    if ((u->type == usage_read_t || u->type == usage_old_read_write_t) && (node_checked_type[u->node] & ifi_unset)) {
      add_uninited_var(u->v);
    }
    vertex_convertions[u->v] = static_cast<is_func_id_t>(vertex_convertions[u->v] | node_checked_type[u->node]);
  }

  std::fill(node_mark_dfs.begin(), node_mark_dfs.end(), UsagePtr());
  std::fill(node_mark_dfs_type_hint.begin(), node_mark_dfs_type_hint.end(), 0);
  for (UsagePtr u : var_split->usage_gen) {
    if (u->type != usage_type_hint_t) {
      dfs_uni_rw_usages(u->node, u);
    }
  }
  for (UsagePtr u : var_split->usage_gen) {
    if (u->type == usage_type_hint_t) {
      dfs_apply_type_hint(u->node, u);
    }
  }

  //fprintf (stdout, "PROCESS:[%s][%d]\n", var->name.c_str(), var->id);

  size_t parts_cnt = 0;
  for (UsagePtr i : var_split->usage_gen) {
    if (node_was[i->node]) {
      UsagePtr u = dsu_get(&var_split->parent, i);
      if (u->part_id == -1) {
        u->part_id = static_cast<int>(parts_cnt++);
      }
    }
  }

  //printf ("parts_cnt = %d\n", parts_cnt);
  if (parts_cnt == 1) {
    return;
  }

  std::vector<std::vector<VertexAdaptor<op_var>>> parts(parts_cnt);
  for (UsagePtr i : var_split->usage_gen) {
    if (node_was[i->node]) {
      UsagePtr u = dsu_get(&var_split->parent, i);
      parts[u->part_id].push_back(i->v);
    }
  }

  split_var(function, var, parts);
}

void CFG::confirm_usage(VertexPtr v, bool recursive_flag) {
  //fprintf (stdout, "%s\n", OpInfo::op_str[v->type()].c_str());
  if (!vertex_usage[v].used || (recursive_flag && !vertex_usage[v].used_rec)) {
    vertex_usage[v].used = true;
    if (recursive_flag) {
      vertex_usage[v].used_rec = true;
      for (auto i : *v) {
        confirm_usage(i, true);
      }
    }
  }
}

void CFG::calc_used(Node v) {
  std::stack<Node> node_stack;
  node_stack.push(v);

  while (!node_stack.empty()) {
    v = node_stack.top();
    node_stack.pop();

    node_was[v] = cur_dfs_mark;
    //fprintf (stdout, "calc_used %d\n", get_index (v));

    for (const auto &node_subtree : node_subtrees[v]) {
      confirm_usage(node_subtree.v, node_subtree.recursive_flag);
    }
    for (Node i : node_next[v]) {
      if (node_was[i] != cur_dfs_mark) {
        node_stack.push(i);
      }
    }
  }
}

class DropUnusedPass final : public FunctionPassBase {
  IdMap<VertexUsage> &vertex_usage;
public:
  string get_description() override {
    return "Drop unused vertices";
  }

  explicit DropUnusedPass(IdMap<VertexUsage> &vertexUsage) :
    vertex_usage(vertexUsage) {}

  VertexPtr on_enter_vertex(VertexPtr v) override {
    if (auto try_op = v.try_as<op_try>()) {
      auto catch_op = try_op->catch_list()[0].as<op_catch>();
      if (!vertex_usage[catch_op->var()].used) {
        kphp_assert(!vertex_usage[catch_op->cmd()].used);
        return try_op->try_cmd();
      }
    }
    if (!vertex_usage[v].used) {
      if (v->type() == op_seq) {
        return VertexAdaptor<op_seq>::create();
      } else {
        return VertexAdaptor<op_empty>::create();
      }
    }
    return v;
  }

  VertexPtr on_exit_vertex(VertexPtr v) override {
    if (auto do_op = v.try_as<op_do>()) {
      if (do_op->cond()->type() == op_empty) {
        do_op->cond() = VertexAdaptor<op_false>::create();
      }
    }
    return v;
  }

};

class AddConversionsPass final : public FunctionPassBase {
  IdMap<is_func_id_t> &conversions;
public:
  string get_description() override {
    return "Add conversions after checks";
  }
  explicit AddConversionsPass(IdMap<is_func_id_t> &conversions) :
    conversions(conversions) {}

  VertexPtr on_exit_vertex(VertexPtr v) override {
    if (v->rl_type != val_r) {
      return v;
    }
    if (auto var = v.try_as<op_var>()) {
      if (!vk::any_of_equal(var->var_id->type(), VarData::var_local_t, VarData::var_param_t) || var->var_id->is_reference || var->var_id->is_foreach_reference) {
        return v;
      }
      //fprintf(stderr, "Variable %s have conv_type %d\n", var->var_id->name.c_str(), conversions[v]);
      if (conversions[v] == 0) {
        kphp_warning(fmt_format("Unreachable code: variable type conditions creates contradiction for variable {}", var->get_string()));
      } else if (conversions[v] == ifi_is_integer) {
        return VertexAdaptor<op_conv_int>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == ifi_is_string) {
        return VertexAdaptor<op_conv_string>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == ifi_is_array) {
        return VertexAdaptor<op_conv_array>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == (ifi_is_bool|ifi_is_false) || conversions[v] == ifi_is_bool) {
        return VertexAdaptor<op_conv_bool>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == ifi_is_float) {
        return VertexAdaptor<op_conv_float>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == ifi_is_null) {
        return VertexAdaptor<op_null>::create().set_rl_type(val_r).set_location(v);
      } else if (conversions[v] == ifi_is_false) {
        return VertexAdaptor<op_false>::create().set_rl_type(val_r).set_location(v);
      } else if ((conversions[v] & (ifi_is_false|ifi_is_null)) == 0) {
        return VertexAdaptor<op_conv_drop_null>::create(VertexAdaptor<op_conv_drop_false>::create(v).set_rl_type(val_r).set_location(v)).set_rl_type(val_r).set_location(v);
      } else if ((conversions[v] & ifi_is_false) == 0) {
        return VertexAdaptor<op_conv_drop_false>::create(v).set_rl_type(val_r).set_location(v);
      } else if ((conversions[v] & ifi_is_null) == 0) {
        return VertexAdaptor<op_conv_drop_null>::create(v).set_rl_type(val_r).set_location(v);
      }
    }
    return v;
  }

  bool user_recursion(VertexPtr root) override {
    // it's a bad idea to add drop_or_false around the parameter definition
    return root->type() == op_func_param_list;
  }
};

int CFG::register_vertices(VertexPtr v, int N) {
  set_index(v, N++);
  for (auto i : *v) {
    N = register_vertices(i, N);
  }
  return N;
}

void CFG::process_function(FunctionPtr function) {
  //vertex_usage
  //var_split_data

  if (function->type != FunctionData::func_local) {
    return;
  }

  auto splittable_vars = collect_splittable_vars(function);
  var_split_data.update_size(static_cast<int>(splittable_vars.size()));
  int var_i = 0;
  for (auto var_id : splittable_vars) {
    set_index(var_id, var_i++);
    var_split_data[var_id] = VarSplitPtr(new VarSplitData());
  }

  int vertex_n = register_vertices(function->root, 0);
  vertex_usage.update_size(vertex_n);
  vertex_convertions.update_size(vertex_n);

  node_gen.add_id_map(&node_next);
  node_gen.add_id_map(&node_prev);
  node_gen.add_id_map(&node_was);
  node_gen.add_id_map(&node_checked_type);
  node_gen.add_id_map(&node_mark_dfs);
  node_gen.add_id_map(&node_mark_dfs_type_hint);
  node_gen.add_id_map(&node_usages);
  node_gen.add_id_map(&node_subtrees);
  cur_dfs_mark = 0;

  Node start, finish;
  create_cfg(function->root, &start, &finish);
  current_start = start;

  cur_dfs_mark++;
  calc_used(start);
  {
    DropUnusedPass pass{vertex_usage};
    run_function_pass(function, &pass);
  }

  for (auto var: splittable_vars) {
    if (var->type() != VarData::var_local_inplace_t) {
      process_var(function, var);
    }
  }

  {
    AddConversionsPass pass{vertex_convertions};
    run_function_pass(function, &pass);
  }

  node_gen.clear();
}
} // namespace cfg

void CFGBeginF::execute(FunctionPtr function, DataStream<FunctionAndCFG> &os) {
  stage::set_name("Calc control flow graph");
  stage::set_function(function);

  cfg::CFG cfg;
  cfg.process_function(function);

  if (stage::has_error()) {
    return;
  }

  os << FunctionAndCFG{function, cfg.move_data()};
}
