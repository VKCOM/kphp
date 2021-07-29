// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-spread-operator.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"

VertexAdaptor<op_array> array_vertex_from_slice(const VertexConstRange &args, size_t start, size_t end) {
  return VertexAdaptor<op_array>::create(
    std::vector<VertexPtr>{args.begin() + start, args.begin() + end}
  );
}

VertexPtr ConvertSpreadOperatorPass::on_exit_vertex(VertexPtr root) {
  if (root->type() != op_array) {
    return root;
  }

  const auto array_vertex = root.try_as<op_array>();
  const auto args = array_vertex->args();
  const auto with_spread = std::any_of(args.begin(), args.end(), [](const auto &arg) {
    return arg->type() == op_spread;
  });

  if (!with_spread) {
    return array_vertex;
  }

  std::vector<VertexPtr> parts;
  constexpr size_t UNINITIALIZED = -1;

  size_t last_spread_index = UNINITIALIZED;
  size_t prev_spread_index = UNINITIALIZED;

  for (int i = 0; i < args.size(); ++i) {
    if (args[i]->type() == op_spread) {
      last_spread_index = i;

      if (prev_spread_index == UNINITIALIZED) {
        const auto has_elements_before = (last_spread_index != 0);

        if (has_elements_before) {
          parts.emplace_back(
            array_vertex_from_slice(args, 0, last_spread_index)
          );
        }
      } else {
        const auto count_elements = last_spread_index - (prev_spread_index + 1);

        if (count_elements != 0) {
          parts.emplace_back(
            array_vertex_from_slice(args, prev_spread_index + 1, last_spread_index)
          );
        }
      }

      parts.emplace_back(args[i].try_as<op_spread>()->expr());

      prev_spread_index = last_spread_index;
    }
  }

  if (last_spread_index != args.size() - 1) {
    parts.emplace_back(
      array_vertex_from_slice(args, last_spread_index + 1, args.size())
    );
  }

  const auto array_merge_spread = G->get_function("array_merge_spread");
  if (!array_merge_spread) {
    return array_vertex;
  }

  auto call = VertexAdaptor<op_func_call>::create(parts).set_location(root->location);
  call->func_id = array_merge_spread;
  call->set_string(array_merge_spread->name);

  return call;
}
