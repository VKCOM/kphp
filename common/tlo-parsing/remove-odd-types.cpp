// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tlo-parsing/remove-odd-types.h"

#include <algorithm>
#include <unordered_set>

#include "common/tlo-parsing/tl-dependency-graph.h"

static void remove_node_types(vk::tl::tl_scheme &scheme, const vk::tl::DependencyGraph &graph, const std::vector<int> &nodes_to_delete) {
  std::unordered_set<const vk::tl::type *> types_to_delete;
  std::unordered_set<const vk::tl::combinator *> functions_to_delete;

  for (int node_id : nodes_to_delete) {
    graph.copy_node_internals_to(node_id, types_to_delete, functions_to_delete);
  }

  std::for_each(types_to_delete.begin(), types_to_delete.end(), [&](const vk::tl::type *t) {
    scheme.remove_type(t);
  });
  std::for_each(functions_to_delete.begin(), functions_to_delete.end(), [&](const vk::tl::combinator *f) {
    scheme.remove_function(f);
  });
}

void vk::tl::remove_cyclic_types(tl_scheme &scheme) {
  vk::tl::DependencyGraph graph{&scheme};
  std::vector<int> cycle_nodes = graph.find_cycles_nodes();
  remove_node_types(scheme, graph, cycle_nodes);
}

void vk::tl::remove_exclamation_types(tl_scheme &scheme) {
  vk::tl::DependencyGraph graph{&scheme};
  std::vector<int> excl_nodes = graph.find_excl_nodes();
  remove_node_types(scheme, graph, excl_nodes);
}
