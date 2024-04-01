// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace vk {
namespace tlo_parsing {

struct tl_scheme;
struct type;
struct combinator;

struct DependencyGraphBuilder;

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
  // TODO: use std::variant here only when engine repo will compiled under c++17 standard,
  // because this header file used there
  union {
    const combinator *combinator_ptr;
    const type *type_ptr;
  } tl_object_ptr_;
};

class DependencyGraph {
public:
  DependencyGraph();
  // Во время использования этого класса необходимо, чтобы *scheme не изменялось
  explicit DependencyGraph(tl_scheme *scheme);
  ~DependencyGraph();

  const std::vector<TLNode> &get_nodes() const;
  // a -> b === a зависит от b
  const std::vector<std::unordered_set<int>> &get_edges() const;
  const std::vector<std::unordered_set<int>> &get_inv_edges() const;
  const TLNode &get_node_info(int node_id) const;

  // Находит все вершины, которые зависят от циклов
  std::vector<int> find_cycles_nodes() const;

  // Находит все вершины, которые зависят от !
  std::vector<int> find_excl_nodes() const;

  std::vector<const vk::tlo_parsing::type *> get_type_dependencies(const type *t) const;
  std::vector<const vk::tlo_parsing::type *> get_function_dependencies(const combinator *f) const;

  std::set<const vk::tlo_parsing::type *> get_weak_self_cyclic_types() const;
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
  std::vector<const vk::tlo_parsing::type *> get_combinator_dependencies(const combinator *c) const;

  friend DependencyGraphBuilder;
};

} // namespace tlo_parsing
} // namespace vk
