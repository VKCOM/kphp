#include "common/tlo-parsing/tl-dependency-graph.h"

#include <algorithm>
#include <unordered_set>

#include "common/tlo-parsing/tl-utils.h"

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
  switch (node.tl_object.index()) {
    case 0:
      c = vk::get<0>(node.tl_object);
      tl_name = c->name;
      break;
    case 1:
      t = vk::get<1>(node.tl_object);
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
  bool weak_dependencies{false};
  bool is_parent_weak{false};

  DependencyGraphBuilder(DependencyGraph &graph, combinator *expr_owner) :
    graph(graph),
    expr_owner_combinator(expr_owner) {}

  void collect_edges(vk::tl::expr_base *expr) {
    weak_dependencies = false;
    is_parent_weak = false;
    expr->visit(*this);
  };
private:
  void apply(vk::tl::type_expr &expr) final {
    type *t = get_type_of(&expr, graph.scheme);
    if (t->is_builtin()) {
      return;
    }
    if (!weak_dependencies) {
      graph.add_edge(TLNode{expr_owner_combinator}, TLNode{t});
    } else {
      graph.combinator_weak_dependencies[expr_owner_combinator].insert(t);
    }
    bool prev_weak_dependencies = weak_dependencies;
    bool prev_is_parent_weak = is_parent_weak;
    if (expr.type_id == TL_MAYBE_ID) {
      weak_dependencies = is_parent_weak;
    } else {
      weak_dependencies = true;
    }
    is_parent_weak = weak_dependencies;
    for (const auto &child : expr.children) {
      child->visit(*this);
    }
    weak_dependencies = prev_weak_dependencies;
    is_parent_weak = prev_is_parent_weak;
  }

  void apply(vk::tl::type_array &array) final {
    weak_dependencies = true;
    for (const auto &arg : array.args) {
      arg->type_expr->visit(*this);
    }
  }

  void apply(vk::tl::type_var &) final {}
  void apply(vk::tl::nat_const &) final {}
  void apply(vk::tl::nat_var &) final {}
};

void vk::tl::DependencyGraph::collect_combinator_edges(vk::tl::combinator *c) {
  DependencyGraphBuilder builder{*this, c};
  for (const auto &arg : c->args) {
    builder.collect_edges(arg->type_expr.get());
  }
  if (c->is_function()) {
    builder.collect_edges(c->result.get());;
  }
}

const std::vector<vk::tl::TLNode> &vk::tl::DependencyGraph::get_nodes() const {
  return nodes;
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

static void normalize_dependencies(vk::tl::Dependencies &deps) {
  static const auto &distinct_vector = [](std::vector<const vk::tl::type *> &vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
  };
  distinct_vector(deps.deps);
  distinct_vector(deps.weak_deps);
  std::vector<const vk::tl::type *> normalised_weak_deps = deps.weak_deps;
  for (const auto &t : deps.deps) {
    auto it = std::find(normalised_weak_deps.begin(), normalised_weak_deps.end(), t);
    if (it != normalised_weak_deps.end()) {
      normalised_weak_deps.erase(it);
    }
  }
  deps.weak_deps = std::move(normalised_weak_deps);
}

vk::tl::Dependencies vk::tl::DependencyGraph::get_type_dependencies(const type *t) const {
  Dependencies res;
  for (const auto &constructor : t->constructors) {
    const auto &ctor_deps = get_combinator_dependencies(constructor.get());
    res.deps.insert(res.deps.end(), ctor_deps.deps.begin(), ctor_deps.deps.end());
    res.weak_deps.insert(res.weak_deps.end(), ctor_deps.weak_deps.begin(), ctor_deps.weak_deps.end());
  }
  normalize_dependencies(res);
  return res;
}

vk::tl::Dependencies vk::tl::DependencyGraph::get_function_dependencies(const combinator *f) const {
  assert(f->is_function());
  return get_combinator_dependencies(f);
}

vk::tl::Dependencies vk::tl::DependencyGraph::get_combinator_dependencies(const combinator *c) const {
  auto it = tl_name_to_id.find(c->name);
  if (it == tl_name_to_id.end()) {
    return {};
  }
  Dependencies res_deps;
  int node_id = it->second;
  const auto &node_deps = edges[node_id];
  res_deps.deps.resize(node_deps.size());
  std::transform(node_deps.begin(), node_deps.end(), res_deps.deps.begin(), [&](int dep_node_id) {
    const auto &dep_node = get_node_info(dep_node_id);
    assert(dep_node.is_type());
    return dep_node.get_type();
  });
  auto ctor_weak_deps_it = combinator_weak_dependencies.find(c);
  if (ctor_weak_deps_it != combinator_weak_dependencies.end()) {
    res_deps.weak_deps.insert(res_deps.weak_deps.end(), ctor_weak_deps_it->second.begin(), ctor_weak_deps_it->second.end());
  }
  normalize_dependencies(res_deps);
  return res_deps;
}
