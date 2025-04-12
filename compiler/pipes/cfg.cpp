// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/cfg.h"

#include <stack>
#include <random>

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"
#include "compiler/debug.h"
#include "compiler/inferring/ifi.h"
#include "compiler/utils/idmap.h"

namespace cfg {
// just simple int id type
class Node {
  DEBUG_STRING_METHOD { return std::to_string(id_); }

  int id_{-1};

public:
  Node() = default;
  explicit Node(int id): id_(id) {}

  explicit operator bool() const { return id_ != -1; }
  void clear() { id_ = -1; }

  friend int get_index(const Node &i) { return i.id_; }
  friend bool operator==(const Node &n1, const Node &n2) { return n1.id_ == n2.id_; }
  friend bool operator!=(const Node &n1, const Node &n2) { return n1.id_ != n2.id_; }
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

  int id;
  int part_id = -1;
  int dsu;            // disjoint set union identifier, see VarSplitData dsu
  Node node;
  UsageType type;
  is_func_id_t checked_type;

  VertexAdaptor<op_var> v;

  UsageData(UsageType type, VertexAdaptor<op_var> v, int id):
    id(id),
    dsu(id),
    type(type),
    v(v) {
  }
};

using UsagePtr = Id<UsageData>;

struct VarSplitData {
  std::vector<UsagePtr> usages;

  VarSplitData() {
    usages.reserve(4);  // an empiric value
  }

  UsagePtr dsu_get(UsagePtr usage) {
    while (usage->dsu != usage->id) {   // non-recursive dsu, works faster than recursive
      usage->dsu = usages[usage->dsu]->dsu;
      usage = usages[usage->dsu];
    }
    return usage;
  }

  void dsu_uni(UsagePtr u1, UsagePtr u2) {
    u1 = dsu_get(u1);
    u2 = dsu_get(u2);
    // naive dsu union (no rand, no size/rank) is pretty ok here
    if (u1->id != u2->id) {
      u1->dsu = u2->id;
    }
  }
};

class CFG {
  CFGData data;
  int n_nodes = 0;
  Node func_root_node;

  // these id maps are node properties WHILE building cfg: every node has them
  // on every new node appearing, their sizes are updated
  // these lists are mostly small or empty for each node, that's why use forward_list instead of vector
  IdMap<std::forward_list<Node>> node_next;
  IdMap<std::forward_list<Node>> node_prev;
  IdMap<std::forward_list<UsagePtr>> node_usages;
  IdMap<std::forward_list<VertexPtr>> node_subvertices;
  void reserve_capacity_for_cfg_idmaps();

  // these is maps are node properties AFTER building cfg: for calculating really used, applying @var phpdocs, etc
  // their sizes are set once after all cfg has been build
  int cur_dfs_step{0};
  IdMap<int> node_dfs;
  IdMap<int> node_dfs_smartcast_mask;
  IdMap<UsagePtr> node_dfs_usages;
  void reserve_size_for_dfs_idmaps();

  IdMap<VarSplitData> var_split_data;

  std::unordered_map<VertexAdaptor<op_var>, is_func_id_t> smartcasts_conversions;

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

  inline Node new_node() __attribute__((always_inline));
  UsagePtr new_usage(UsageType type, VertexAdaptor<op_var> v);
  void add_usage(Node node, UsagePtr usage);
  void add_subtree(Node node, VertexPtr subtree_vertex, bool recursive_flag);
  void add_edge(Node from, Node to);
  void collect_ref_vars(VertexPtr v, std::unordered_set<VarPtr> &ref);
  std::vector<VarPtr> collect_splittable_vars(FunctionPtr func);
  void create_cfg(VertexPtr tree_node, Node *res_start, Node *res_finish, bool write_flag = false, bool weak_write_flag = false);
  void create_condition_cfg(VertexPtr tree_node, Node *res_start, Node *res_true, Node *res_false);

  void calc_used(Node v);

  void dfs_apply_smartcasts(Node v, VarPtr var, int current_mask);

  bool try_uni_usages(UsagePtr usage, UsagePtr another_usage);
  void dfs_uni_rw_usages(Node v, UsagePtr usage);
  void dfs_apply_type_hint(Node v, UsagePtr type_hint_usage);
  bool dfs_is_uninited_usage(Node v, UsagePtr read_usage);
  void process_var(FunctionPtr function, VarPtr v);
  void on_uninited_var(VertexAdaptor<op_var> v);
  void split_var(FunctionPtr function, VarPtr var, std::vector<std::vector<VertexAdaptor<op_var>>> &parts);
public:
  void process_function(FunctionPtr func);
  CFGData move_data() {
    return std::move(data);
  }
};

inline Node CFG::new_node() {
  Node res(n_nodes++);

  if ((n_nodes & 0x3f) == 1) {
    reserve_capacity_for_cfg_idmaps();
  }

  return res;
}

void CFG::reserve_capacity_for_cfg_idmaps() {
  int capacity = (n_nodes | 0x3f) + 1;
  node_prev.update_size(capacity);
  node_next.update_size(capacity);
  node_usages.update_size(capacity);
  node_subvertices.update_size(capacity);
}

void CFG::reserve_size_for_dfs_idmaps() {
  node_dfs.update_size(n_nodes);
  node_dfs_smartcast_mask.update_size(n_nodes);
  node_dfs_usages.update_size(n_nodes);
}

UsagePtr CFG::new_usage(UsageType type, VertexAdaptor<op_var> v) {
  VarPtr var = v->var_id;
  kphp_assert (var);
  if (get_index(var) < 0) {    // non-splittable var
    return {};
  }
  VarSplitData &var_split = var_split_data[var];
  UsagePtr res = UsagePtr(new UsageData(type, v, var_split.usages.size()));
  var_split.usages.emplace_back(res);
  return res;
}

void CFG::add_usage(Node node, UsagePtr usage) {
  if (!usage) {
    return;
  }
  //fprintf(stderr, "%s is used at node %d with type %d\n", usage->v->get_string().c_str(), get_index(node), usage->type);
  //hope that one node will contain usages of the same type
  kphp_assert (node_usages[node].empty() || node_usages[node].front()->type == usage->type);
  node_usages[node].emplace_front(usage);
  usage->node = node;
}

void CFG::add_subtree(Node node, VertexPtr subtree_vertex, bool recursive_flag) {
  kphp_assert (node && subtree_vertex);
  node_subvertices[node].emplace_front(subtree_vertex);
  
  if (recursive_flag) {
    for (VertexPtr v : *subtree_vertex) {
      add_subtree(node, v, true);
    }
  }
}

void CFG::add_edge(Node from, Node to) {
  if (from && to) {
    //fprintf(stderr, "%s, add-edge: %d->%d\n", stage::get_function_name().c_str(), get_index(from), get_index(to));
    node_next[from].emplace_front(to);
    node_prev[to].emplace_front(from);
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
    if (ref_vars.find(var) == ref_vars.end() && var->type() != VarData::var_local_inplace_t) {
      splittable_vars.emplace_back(var);
    }
  }

  for (VarPtr var : func->param_ids) {
    auto param = params[var->param_i].as<op_func_param>();
    if (!param->var()->ref_flag && var->name != "this") {
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
        bool can_var_be_smartcasted = get_index(var->var_id) >= 0;
        if (can_var_be_smartcasted) {
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
      } else if (tree_node->type() == op_set) {
        // support for cases like: if ($x = $some_value) {...}
        if (auto var = tree_node.try_as<meta_op_binary>()->lhs().try_as<op_var>()) {
          add_type_check_usage(*res_true, ifi_any_type & ~(ifi_is_false | ifi_is_null), var);
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
    case op_string_build:
    case op_callback_of_builtin: {
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
    case op_ffi_c2php_conv:
    case op_ffi_php2c_conv:
    case op_ffi_addr:
    case op_ffi_cdata_value_ref:
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

    case op_ffi_cast: {
      auto op = tree_node.as<op_ffi_cast>();
      if (op->has_array_size_expr()) {
        Node expr_end, array_size_expr_start;
        create_cfg(op->expr(), res_start, &expr_end);
        create_cfg(op->array_size_expr(), &array_size_expr_start, res_finish);
        add_edge(expr_end, array_size_expr_start);
      } else {
        create_cfg(op->expr(), res_start, res_finish);
      }
      break;
    }

    case op_ffi_array_get: {
      auto op = tree_node.as<op_ffi_array_get>();
      Node array_end, key_start;
      create_cfg(op->array(), res_start, &array_end);
      create_cfg(op->key(), &key_start, res_finish);
      add_edge(array_end, key_start);
      break;
    }

    case op_ffi_array_set: {
      auto op = tree_node.as<op_ffi_array_set>();
      Node array_end, key_start, key_end, value_start;
      create_cfg(op->array(), res_start, &array_end, false, write_flag || weak_write_flag);
      create_cfg(op->key(), &key_start, &key_end);
      create_cfg(op->value(), &value_start, res_finish);
      add_edge(array_end, key_start);
      add_edge(key_end, value_start);
      break;
    }

    case op_ffi_new: {
      auto op = tree_node.as<op_ffi_new>();
      Node owned_flag_expr_end, array_size_expr_start;
      create_cfg(op->owned_flag_expr(), res_start, &owned_flag_expr_end);
      if (op->has_array_size_expr()) {
        create_cfg(op->array_size_expr(), &array_size_expr_start, res_finish);
        add_edge(owned_flag_expr_end, array_size_expr_start);
      } else {
        *res_finish = owned_flag_expr_end;
      }
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
    case op_ffi_load_call:
      create_cfg(tree_node.as<op_ffi_load_call>()->func_call(), res_start, res_finish);
      break;
    case op_fork: {
      create_cfg(tree_node.as<op_fork>()->func_call(), res_start, res_finish);
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
        add_subtree(catch_start, catch_op->cmd(), true);  // todo shall we pass true here?
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
    VarSplitData &var_split = var_split_data[var];
    var_split.dsu_uni(usage, another_usage);
    return true;
  }
  return false;
}

// usages unification is used for variable splitting
// for example: function f() { $a = 1; f1($a); f2($a); $a = '2'; f3($a); } — 5 usages (wrrwr), will be unified 123 and 45
// here, having a particular usage we unify traversing up all available paths until a write usage found
// (of course, mind cases like { $a = 1; if(rand()) $a = 2; f1($a); } — these all (wwr) will be unified into a single)
void CFG::dfs_uni_rw_usages(Node v, UsagePtr usage) {
  UsagePtr other_usage = node_dfs_usages[v];
  if (other_usage) {
    try_uni_usages(usage, other_usage);
    return;
  }
  node_dfs_usages[v] = usage;

  for (UsagePtr another_usage : node_usages[v]) {
    if (try_uni_usages(usage, another_usage) && another_usage->type == usage_write_t) {
      return;   // stop when we reached a write usage traversing up
    }
  }

  for (Node i : node_prev[v]) {
    dfs_uni_rw_usages(i, usage);
  }
}

// apply @var phpdoc: starting from a given point, it's applied all the way down (even unifying write usages)
// for example: function f() { /** @var int $x */ $x = 1; $x = '2'; } all these will be unified, $x won't be splitted
// only another type hint can stop unification, like /** @var int */ $x = 1; /** @var string */ $x = '2';
void CFG::dfs_apply_type_hint(Node v, UsagePtr type_hint_usage) {
  UsagePtr other_usage = node_dfs_usages[v];
  if (other_usage && other_usage->type != usage_type_hint_t) {
    try_uni_usages(type_hint_usage, other_usage);
  }
  if (node_dfs[v] == cur_dfs_step) {
    return;
  }
  node_dfs_usages[v] = type_hint_usage;
  node_dfs[v] = cur_dfs_step;

  for (UsagePtr another_usage : node_usages[v]) {
    if (another_usage->type == usage_type_hint_t && type_hint_usage != another_usage && another_usage->v->var_id == type_hint_usage->v->var_id) {
      return;   // stop when we reached another type hint traversing down
    }
    try_uni_usages(type_hint_usage, another_usage);
  }

  for (Node i : node_next[v]) {
    dfs_apply_type_hint(i, type_hint_usage);
  }
}

// test whether a read usage operates potentially uninited var
// for example, function f() { if(1) $a = 1; echo $a; f1($a); } — usages 2 and 3 are potentially uninited
// (but! we'll trigger an error only for the first uninited usage (2 in this case), not for all)
// here, having a read usage, we traverse up all available paths stopping and write usages
// if we reach function root — it's an uninited usage
bool CFG::dfs_is_uninited_usage(Node v, UsagePtr read_usage) {
  node_dfs[v] = cur_dfs_step;

  for (UsagePtr another_usage : node_usages[v]) {
    if (another_usage->v->var_id == read_usage->v->var_id && another_usage->type == usage_write_t) {
      return false;
    }
  }

  for (Node i : node_prev[v]) {
    int prev_dfs_step = node_dfs[i];
    if (prev_dfs_step != 0 && prev_dfs_step != cur_dfs_step && dfs_is_uninited_usage(i, read_usage)) {
      return true;
    }
    // prev_dfs_step != 0 means that node is used (see calc_used() which is invoked before vars processing)
    // if prev_dfs_step == cur_dfs_step, then this usage might be also uninited,
    // but don't recurse once again, because if yes, we have already triggered an uninited error for that (prev) usage
  }
  return v == func_root_node;
}

// dfs for smart casts calculates narrowed types of var for some usages
// for example, function f($v) { if(is_int($v)) fInt($v); }
// note, that unlike other dfs functions, it accepts VarPtr, not UsagePtr,
// and calculates node_dfs_smartcast_mask for var throughout all reachable cfg nodes
// it starts from a function root with mask ifi_any_type and traverses all the cfg tree down narrowing current_mask
// so, a node can be traversed multiple times if it is reachable with different masks
void CFG::dfs_apply_smartcasts(Node v, VarPtr var, int current_mask) {
  if ((node_dfs_smartcast_mask[v] | current_mask) == node_dfs_smartcast_mask[v]) {
    return;
  }
  node_dfs_smartcast_mask[v] = node_dfs_smartcast_mask[v] | current_mask;

  for (UsagePtr another_usage : node_usages[v]) {
    if (another_usage->v->var_id == var) {
      if (another_usage->type == usage_write_t || another_usage->type == usage_weak_write_t) {
        current_mask = ifi_any_type;
      } else if (another_usage->type == usage_type_check_t) {   // is_int($v), $v !== false, etc
        current_mask &= another_usage->checked_type;
      }
    }
  }

  for (Node i : node_next[v]) {
    dfs_apply_smartcasts(i, var, current_mask);
  }
}

void CFG::on_uninited_var(VertexAdaptor<op_var> v) {
  // when we met an uninited usage — don't emit a warning here: instead, save it and analyze after tinf
  data.uninited_vars.emplace_front(v);
  v->var_id->set_uninited_flag(true);
}

void CFG::split_var(FunctionPtr function, VarPtr var, std::vector<std::vector<VertexAdaptor<op_var>>> &parts) {
  kphp_assert(var->type() == VarData::var_local_t || var->type() == VarData::var_param_t);
  if (parts.empty()) {
    if (var->type() == VarData::var_local_t) {
      function->local_var_ids.erase(std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var));
    }
    return;
  }
  kphp_assert(parts.size() > 1);

  // don't split arguments that have assumptions: function (A $a) { $a = new B; } is unsplittable: it's assumed as A
  // (but we had to collect such arguments in collect_splittable_vars() to make smartcasts and nullability work)
  if (var->type() == VarData::var_param_t && function->get_assumption_for_var(var->name)) {
    return;
  }

  for (size_t i = 0; i < parts.size(); i++) {
    // name$v1, name$v2 and so on, but name (0th copy) is kept as is
    const std::string &new_name = i ? var->name + "$v" + std::to_string(i) : var->name;
    VarPtr new_var = G->create_var(new_name, var->type());
    new_var->holder_func = var->holder_func;

    for (auto v : parts[i]) {
      v->var_id = new_var;
    }

    VertexRange params = function->get_params();
    if (var->type() == VarData::var_local_t) {
      function->local_var_ids.push_back(new_var);

    } else {    // var_param_t
      bool is_param = std::find(parts[i].begin(), parts[i].end(), params[var->param_i].as<op_func_param>()->var()) != parts[i].end();

      if (is_param) { //union of part that contains function argument
        new_var->param_i = var->param_i;
        new_var->init_val = var->init_val;
        new_var->is_read_only = var->is_read_only;
        new_var->tinf_node.init_as_argument(new_var);
        function->param_ids[var->param_i] = new_var;
      } else {
        new_var->type() = VarData::var_local_t;
        function->local_var_ids.push_back(new_var);
      }
    }
  }

  if (var->type() == VarData::var_local_t) {
    function->local_var_ids.erase(std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var));
  }

  data.split_parts_list.emplace_front(std::move(parts));
}

void CFG::process_var(FunctionPtr function, VarPtr var) {
  VarSplitData &var_split = var_split_data[var];

  bool potentially_has_smartcasts = false;
  for (UsagePtr u : var_split.usages) {
    if (u->type == usage_type_check_t) {
      potentially_has_smartcasts = true;
      break;
    }
  }
  if (potentially_has_smartcasts) {
    std::fill(node_dfs_smartcast_mask.begin(), node_dfs_smartcast_mask.end(), 0);
    dfs_apply_smartcasts(func_root_node, var, ifi_any_type);
    for (UsagePtr u : var_split.usages) {
      if (u->type == usage_read_t && node_dfs_smartcast_mask[u->node] != ifi_any_type && node_dfs[u->node]) {
        smartcasts_conversions[u->v] = static_cast<is_func_id_t>(node_dfs_smartcast_mask[u->node]);
      }
    }
  }

  if (var->type() != VarData::var_param_t) {
    cur_dfs_step++;
    for (UsagePtr u : var_split.usages) {
      if (u->type == usage_read_t || u->type == usage_weak_write_t) {
        if (node_dfs[u->node] && dfs_is_uninited_usage(u->node, u)) {
          on_uninited_var(u->v);
          break;  // fire only the first uninited usage for a variable, not all usages
        }
      }
    }
  }

  cur_dfs_step++;
  std::fill(node_dfs_usages.begin(), node_dfs_usages.end(), UsagePtr());
  for (UsagePtr u : var_split.usages) {
    if (u->type != usage_type_hint_t) {
      dfs_uni_rw_usages(u->node, u);
    }
  }
  for (UsagePtr u : var_split.usages) {
    if (u->type == usage_type_hint_t) {
      dfs_apply_type_hint(u->node, u);
    }
  }

  size_t parts_cnt = 0;
  for (UsagePtr i : var_split.usages) {
    if (node_dfs[i->node]) {    // is used (has ever occurred in dfs searching)
      UsagePtr u = var_split.dsu_get(i);
      if (u->part_id == -1) {
        u->part_id = static_cast<int>(parts_cnt++);
      }
    }
  }

  if (parts_cnt == 1) {
    return;
  }

  std::vector<std::vector<VertexAdaptor<op_var>>> parts(parts_cnt);
  for (UsagePtr i : var_split.usages) {
    if (node_dfs[i->node]) {
      UsagePtr u = var_split.dsu_get(i);
      parts[u->part_id].push_back(i->v);
    }
  }

  split_var(function, var, parts);
}

void CFG::calc_used(Node v) {
  std::stack<Node> node_stack;
  node_stack.push(v);

  while (!node_stack.empty()) {
    v = node_stack.top();
    node_stack.pop();

    node_dfs[v] = cur_dfs_step;
    //fprintf (stdout, "calc_used %d\n", get_index (v));

    for (VertexPtr node_subvertex : node_subvertices[v]) {
      node_subvertex->used_flag = true;
    }
    for (Node i : node_next[v]) {
      if (node_dfs[i] != cur_dfs_step) {
        node_stack.push(i);
      }
    }
  }
}

class DropUnusedPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Drop unused vertices";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override {
    if (auto try_op = v.try_as<op_try>()) {
      auto catch_op = try_op->catch_list()[0].as<op_catch>();
      if (!catch_op->var()->used_flag) {
        kphp_assert(!catch_op->cmd()->used_flag);
        return try_op->try_cmd();
      }
    }
    if (!v->used_flag) {
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

class AddSmartcastsConversionsPass final : public FunctionPassBase {
  std::unordered_map<VertexAdaptor<op_var>, is_func_id_t> &conversions;
public:
  std::string get_description() override {
    return "Add conversions after checks";
  }
  explicit AddSmartcastsConversionsPass(std::unordered_map<VertexAdaptor<op_var>, is_func_id_t> &conversions) :
    conversions(conversions) {}

  VertexPtr on_exit_vertex(VertexPtr v) override {
    if (v->rl_type != val_r) {
      return v;
    }
    if (auto var = v.try_as<op_var>()) {
      if (!vk::any_of_equal(var->var_id->type(), VarData::var_local_t, VarData::var_param_t) || var->var_id->is_reference || var->var_id->is_foreach_reference) {
        return v;
      }
      const auto &it = conversions.find(var);
      if (it == conversions.end()) {
        return v;
      }
      is_func_id_t conv = it->second;
      //fprintf(stderr, "Variable %s have conv_type %d\n", var->var_id->name.c_str(), conv);

      if (conv == 0) {
        kphp_warning(fmt_format("Unreachable code: variable type conditions creates contradiction for variable {}", var->get_string()));
      } else if (conv == ifi_is_integer) {
        return VertexAdaptor<op_conv_int>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conv == ifi_is_string) {
        return VertexAdaptor<op_conv_string>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conv == ifi_is_array) {
        return VertexAdaptor<op_conv_array>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conv == (ifi_is_bool|ifi_is_false) || conv == ifi_is_bool) {
        return VertexAdaptor<op_conv_bool>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conv == ifi_is_float) {
        return VertexAdaptor<op_conv_float>::create(v).set_rl_type(val_r).set_location(v);
      } else if (conv == ifi_is_null) {
        return VertexAdaptor<op_null>::create().set_rl_type(val_r).set_location(v);
      } else if (conv == ifi_is_false) {
        return VertexAdaptor<op_false>::create().set_rl_type(val_r).set_location(v);
      } else if ((conv & (ifi_is_false|ifi_is_null)) == 0) {
        return VertexAdaptor<op_conv_drop_null>::create(VertexAdaptor<op_conv_drop_false>::create(v).set_rl_type(val_r).set_location(v)).set_rl_type(val_r).set_location(v);
      } else if ((conv & ifi_is_false) == 0) {
        return VertexAdaptor<op_conv_drop_false>::create(v).set_rl_type(val_r).set_location(v);
      } else if ((conv & ifi_is_null) == 0) {
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

void CFG::process_function(FunctionPtr function) {
  auto splittable_vars = collect_splittable_vars(function);
  var_split_data.update_size(static_cast<int>(splittable_vars.size()));
  int var_id = 0;
  for (auto var : splittable_vars) {
    set_index(var, var_id++);
  }

  Node start, finish;
  create_cfg(function->root, &start, &finish);
  reserve_size_for_dfs_idmaps();
  func_root_node = start;

  cur_dfs_step++;
  calc_used(start);
  {
    DropUnusedPass pass;
    run_function_pass(function, &pass);
  }

  for (auto var: splittable_vars) {
    process_var(function, var);
  }

  if (!smartcasts_conversions.empty()) {
    AddSmartcastsConversionsPass pass{smartcasts_conversions};
    run_function_pass(function, &pass);
  }

  // all idmaps will be auto cleared in their destructors
}
} // namespace cfg

void CFGBeginF::execute(FunctionPtr function, DataStream<FunctionAndCFG> &os) {
  stage::set_name("Calc control flow graph");
  stage::set_function(function);

  cfg::CFG cfg;
  if (function->type == FunctionData::func_local || function->type == FunctionData::func_lambda) {
    cfg.process_function(function);
  }

  if (stage::has_error()) {
    return;
  }

  os << FunctionAndCFG{function, cfg.move_data()};
}
