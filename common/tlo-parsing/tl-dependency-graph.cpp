// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tlo-parsing/tl-dependency-graph.h"

#include <algorithm>
#include <set>
#include <unordered_set>

#include "common/algorithms/find.h"
#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {

struct TLNode {
  enum {
    holds_combinator,
    holds_type,
  } node_type;

  explicit TLNode(const combinator *c) :
    node_type(holds_combinator) {
    tl_object_ptr_.combinator_ptr = c;
  }

  explicit TLNode(const type *t) :
    node_type(holds_type) {
    tl_object_ptr_.type_ptr = t;
  }

  bool is_type() const {
    return node_type == holds_type;
  }

  bool is_combinator() const {
    return node_type == holds_combinator;
  }

  const type *get_type() const {
    return tl_object_ptr_.type_ptr;
  }

  const combinator *get_combinator() const {
    return tl_object_ptr_.combinator_ptr;
  }

private:
  // TODO when we switch to C++17, use std::variant
  union {
    const combinator *combinator_ptr;
    const type *type_ptr;
  } tl_object_ptr_;
};

} // namespace tl
} // namespace vk

vk::tl::DependencyGraph::DependencyGraph(vk::tl::tl_scheme *scheme) : scheme(scheme) {
  const size_t approximate_nodes_cnt = 2 * scheme->types.size() + scheme->functions.size();
  nodes.reserve(approximate_nodes_cnt);
  edges.reserve(approximate_nodes_cnt);

  for (const auto &type_item : scheme->types) {
    type *type = type_item.second.get();
    register_node(TLNode{type});
    for (const auto &constructor : type->constructors) {
      add_edge(TLNode{type}, TLNode{constructor.get()});
      collect_combinator_edges(constructor.get());
    }
  }
  for (const auto &function_item : scheme->functions) {
    combinator *function = function_item.second.get();
    register_node(TLNode{function});
    collect_combinator_edges(function);
  }
}

vk::tl::DependencyGraph::~DependencyGraph() = default;

void vk::tl::DependencyGraph::add_edge(const TLNode &from, const TLNode &to) {
  int from_id = register_node(from);
  int to_id = register_node(to);
  if (from_id == to_id) {
    return;
  }
  edges[from_id].insert(to_id);
  inv_edges[to_id].insert(from_id);
}

int vk::tl::DependencyGraph::register_node(const TLNode &node) {
  std::string tl_name;
  const combinator *c = nullptr;
  const type *t = nullptr;
  switch (node.node_type) {
    case TLNode::holds_combinator:
      c = node.get_combinator();
      tl_name = c->name;
      break;
    case TLNode::holds_type:
      t = node.get_type();
      tl_name = t->name;
      break;
    default:
      assert(false);
  }
  int node_id;
  auto it = tl_name_to_id.find(tl_name);
  if (it == tl_name_to_id.end()) {
    node_id = nodes.size();
    if (t) {
      nodes.emplace_back(t);
    } else {
      nodes.emplace_back(c);
    }
    tl_name_to_id[tl_name] = node_id;
    edges.emplace_back();
    inv_edges.emplace_back();
  } else {
    node_id = it->second;
  }
  return node_id;
}

struct vk::tl::DependencyGraphBuilder : private vk::tl::expr_visitor {
  DependencyGraph &graph;
  combinator *expr_owner_combinator;
  std::set<const type *> weak_self_cyclic_types;
  bool treat_dep_as_weak{false}; // it means that pointer is used => forward declaration is enough

  DependencyGraphBuilder(DependencyGraph &graph, combinator *expr_owner) :
    graph(graph),
    expr_owner_combinator(expr_owner) {}

  void collect_edges(vk::tl::expr_base *expr) {
    expr->visit(*this);
  };
private:
  void apply(vk::tl::type_expr &expr) final {
    type *t = get_type_of(&expr, graph.scheme);
    if (t->is_builtin()) {
      return;
    }
    if (treat_dep_as_weak && expr_owner_combinator->is_constructor() && get_type_of(expr_owner_combinator, graph.scheme)->name == t->name) {
      weak_self_cyclic_types.insert(t);
    } else {
      graph.add_edge(TLNode{expr_owner_combinator}, TLNode{t});
    }
    bool new_treat_dep_as_weak = treat_dep_as_weak || is_pointer_used_in_child(t);
    std::swap(treat_dep_as_weak, new_treat_dep_as_weak);
    for (const auto &child : expr.children) {
      child->visit(*this);
    }
    std::swap(new_treat_dep_as_weak, treat_dep_as_weak);
  }

  void apply(vk::tl::type_array &array) final {
    treat_dep_as_weak = true;
    for (const auto &arg : array.args) {
      arg->type_expr->visit(*this);
    }
  }

  void apply(vk::tl::type_var &) final {}
  void apply(vk::tl::nat_const &) final {}
  void apply(vk::tl::nat_var &) final {}

  static bool is_pointer_used_in_child(const type *t) {
    return vk::any_of_equal(t->name, "Vector", "Dictionary", "IntKeyDictionary", "LongKeyDictionary");
  }
};

void vk::tl::DependencyGraph::collect_combinator_edges(vk::tl::combinator *c) {
  DependencyGraphBuilder builder{*this, c};
  for (const auto &arg : c->args) {
    builder.collect_edges(arg->type_expr.get());
  }
  if (c->is_function()) {
    builder.collect_edges(c->result.get());;
  }
  weak_self_cyclic_types.insert(builder.weak_self_cyclic_types.begin(), builder.weak_self_cyclic_types.end());
}

const std::vector<std::unordered_set<int>> &vk::tl::DependencyGraph::get_edges() const {
  return edges;
}

const std::vector<std::unordered_set<int>> &vk::tl::DependencyGraph::get_inv_edges() const {
  return inv_edges;
}

const vk::tl::TLNode &vk::tl::DependencyGraph::get_node_info(int node_id) const {
  assert(node_id < nodes.size());
  return nodes[node_id];
}

void vk::tl::DependencyGraph::copy_node_internals_to(int node_id, std::unordered_set<const vk::tl::type *> &types_out,
                                                     std::unordered_set<const vk::tl::combinator *> &functions_out) const {
  const vk::tl::TLNode &node = get_node_info(node_id);
  if (node.is_combinator()) {
    auto *c = node.get_combinator();
    if (c->is_function()) {
      functions_out.insert(c);
    } else if (c->is_constructor()) {
      vk::tl::type *t = get_type_of(c, scheme);
      types_out.insert(t);
    }
  } else if (node.is_type()) {
    types_out.insert(node.get_type());
  }
}

std::vector<int> vk::tl::DependencyGraph::find_cycles_nodes() const {
  std::vector<int> used(nodes.size());
  std::vector<bool> reachable_from_cycles(nodes.size());
  for (int v = 0; v < nodes.size(); ++v) {
    if (!used.at(v)) {
      dfs(v, used, false, &reachable_from_cycles);
    }
  }

  for (auto &item : used) {
    item = 0;
  }
  for (int v = 0; v < nodes.size(); ++v) {
    if (reachable_from_cycles.at(v) && !used.at(v)) {
      dfs(v, used, true);
    }
  }
  std::vector<int> res_ids;
  for (int v = 0; v < nodes.size(); ++v) {
    if (used.at(v)) {
      res_ids.emplace_back(v);
    }
  }
  return res_ids;
}

std::vector<int> vk::tl::DependencyGraph::find_excl_nodes() const {
  static auto is_node_exclamation = [&](int node_id) {
    auto node = get_node_info(node_id);
    if (node.is_combinator()) {
      const combinator *c = node.get_combinator();
      return std::any_of(c->args.begin(), c->args.end(), [](const std::unique_ptr<arg> &arg) {
        return arg->is_forwarded_function();
      });
    } else {
      assert(node.is_type());
      const type *t = node.get_type();
      return std::any_of(t->constructors.begin(), t->constructors.end(), [](const std::unique_ptr<combinator> &c) {
        return std::any_of(c->args.begin(), c->args.end(), [](const std::unique_ptr<arg> &arg) {
          return arg->is_forwarded_function();
        });
      });
    }
  };
  std::vector<int> used(nodes.size());
  for (int v = 0; v < nodes.size(); ++v) {
    if (is_node_exclamation(v) && !used.at(v)) {
      dfs(v, used, true);
    }
  }
  std::vector<int> res_ids;
  for (int v = 0; v < nodes.size(); ++v) {
    if (used.at(v)) {
      res_ids.emplace_back(v);
    }
  }
  return res_ids;
}

void vk::tl::DependencyGraph::dfs(int node, std::vector<int> &used, bool use_inv_edges, std::vector<bool> *reachable_from_cycles) const {
  static constexpr int NODE_UNVISITED = 0;
  static constexpr int NODE_ENTERED = 1;
  static constexpr int NODE_EXITED = 2;

  used.at(node) = NODE_ENTERED;
  const auto &edges_to_go = use_inv_edges ? inv_edges : edges;
  for (const auto &to : edges_to_go.at(node)) {
    switch (used.at(to)) {
      case NODE_UNVISITED:
        dfs(to, used, use_inv_edges, reachable_from_cycles);
        break;
      case NODE_ENTERED:
        if (reachable_from_cycles) {
          reachable_from_cycles->at(to) = true;
        }
        break;
      case NODE_EXITED:
        break;
      default:
        assert(false);
    }
  }
  used.at(node) = NODE_EXITED;
}

static void normalize_dependencies(std::vector<const vk::tl::type *> &deps) {
  static const auto &distinct_vector = [](std::vector<const vk::tl::type *> &vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  };
  distinct_vector(deps);
}

std::vector<const vk::tl::type *> vk::tl::DependencyGraph::get_type_dependencies(const type *t) const {
  std::vector<const vk::tl::type *> deps;
  for (const auto &constructor : t->constructors) {
    const auto &ctor_deps = get_combinator_dependencies(constructor.get());
    deps.insert(deps.end(), ctor_deps.begin(), ctor_deps.end());
  }
  normalize_dependencies(deps);
  return deps;
}

std::vector<const vk::tl::type *> vk::tl::DependencyGraph::get_function_dependencies(const combinator *f) const {
  assert(f->is_function());
  return get_combinator_dependencies(f);
}

std::vector<const vk::tl::type *> vk::tl::DependencyGraph::get_combinator_dependencies(const combinator *c) const {
  auto it = tl_name_to_id.find(c->name);
  if (it == tl_name_to_id.end()) {
    return {};
  }
  std::vector<const vk::tl::type *>  deps;
  int node_id = it->second;
  const auto &node_deps = edges[node_id];
  deps.resize(node_deps.size());
  std::transform(node_deps.begin(), node_deps.end(), deps.begin(), [&](int dep_node_id) {
    const auto &dep_node = get_node_info(dep_node_id);
    assert(dep_node.is_type());
    return dep_node.get_type();
  });
  normalize_dependencies(deps);
  return deps;
}

std::set<const vk::tl::type *> vk::tl::DependencyGraph::get_weak_self_cyclic_types() const {
  return weak_self_cyclic_types;
}

bool vk::tl::DependencyGraph::is_type_weak_self_cyclic(const vk::tl::type *t) const {
  return weak_self_cyclic_types.count(t);
}
