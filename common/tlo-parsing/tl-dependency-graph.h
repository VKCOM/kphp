// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <unordered_set>

#include "common/tlo-parsing/tl-objects.h"
#include "common/wrappers/variant.h"

namespace vk {
namespace tl {

struct TLNode {
  vk::variant<const combinator *, const type *> tl_object;

  explicit TLNode(const combinator *c) :
    tl_object(c) {}
  explicit TLNode(const type *t) :
    tl_object(t) {}

  bool is_type() const {
    return vk::holds_alternative<const type *>(tl_object);
  }

  bool is_combinator() const {
    return vk::holds_alternative<const combinator *>(tl_object);
  }

  const type *get_type() const {
    return vk::get<const type *>(tl_object);
  }

  const combinator *get_combinator() const {
    return vk::get<const combinator *>(tl_object);
  }
};

struct DependencyGraphBuilder;

class DependencyGraph {
public:
  DependencyGraph() = default;

  // Во время использования этого класса необходимо, чтобы *scheme не изменялось
  explicit DependencyGraph(tl_scheme *scheme);

  const std::vector<TLNode> &get_nodes() const;
  // a -> b === a зависит от b
  const std::vector<std::unordered_set<int>> &get_edges() const;
  const std::vector<std::unordered_set<int>> &get_inv_edges() const;
  const TLNode &get_node_info(int node_id) const;

  // Находит все вершины, которые зависят от циклов
  std::vector<int> find_cycles_nodes() const;

  // Находит все вершины, которые зависят от !
  std::vector<int> find_excl_nodes() const;

  std::vector<const vk::tl::type *> get_type_dependencies(const type *t) const;
  std::vector<const vk::tl::type *> get_function_dependencies(const combinator *f) const;

  std::set<const vk::tl::type *> get_weak_self_cyclic_types() const;
  bool is_type_weak_self_cyclic(const type *t) const;
private:
  tl_scheme *scheme{};
  std::unordered_map<std::string, int> tl_name_to_id;
  std::vector<TLNode> nodes;
  std::vector<std::unordered_set<int>> edges;
  std::vector<std::unordered_set<int>> inv_edges;

  std::set<const type *> weak_self_cyclic_types;

  void dfs(int node, std::vector<int> &used, bool use_inv_edges, std::vector<bool> *reachable_from_cycles = nullptr) const;
  void collect_combinator_edges(combinator *c);
  void add_edge(const TLNode &from, const TLNode &to);
  int register_node(const TLNode &node);
  std::vector<const vk::tl::type *> get_combinator_dependencies(const combinator *c) const;

  friend DependencyGraphBuilder;
};
}
}
