// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <unordered_set>

#include "common/tlo-parsing/tl-dependency-graph.h"
#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"

static void remove_node_types(vk::tlo_parsing::tl_scheme &scheme, const vk::tlo_parsing::DependencyGraph &graph, const std::vector<int> &nodes_to_delete) {
  std::unordered_set<const vk::tlo_parsing::type *> types_to_delete;
  std::unordered_set<const vk::tlo_parsing::combinator *> functions_to_delete;

  for (int node_id : nodes_to_delete) {
    graph.copy_node_internals_to(node_id, types_to_delete, functions_to_delete);
  }

  std::for_each(types_to_delete.begin(), types_to_delete.end(), [&](const vk::tlo_parsing::type *t) {
    scheme.remove_type(t);
  });
  std::for_each(functions_to_delete.begin(), functions_to_delete.end(), [&](const vk::tlo_parsing::combinator *f) {
    scheme.remove_function(f);
  });
}

void vk::tlo_parsing::remove_cyclic_types(tl_scheme &scheme) {
  vk::tlo_parsing::DependencyGraph graph{&scheme};
  std::vector<int> cycle_nodes = graph.find_cycles_nodes();
  remove_node_types(scheme, graph, cycle_nodes);
}

void vk::tlo_parsing::remove_exclamation_types(tl_scheme &scheme) {
  vk::tlo_parsing::DependencyGraph graph{&scheme};
  std::vector<int> excl_nodes = graph.find_excl_nodes();
  remove_node_types(scheme, graph, excl_nodes);
}
