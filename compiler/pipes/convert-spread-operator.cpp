// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-spread-operator.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"

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

  int last_spread_index = -1;
  int prev_spread_index = -1;
  for (int i = 0; i < args.size(); ++i) {
    if (args[i]->type() == op_spread) {
      last_spread_index = i;

      if (prev_spread_index == -1) {
        if (last_spread_index != 0) {
          parts.emplace_back(
            VertexAdaptor<op_array>::create(
              std::vector<VertexPtr>{args.begin(), args.begin() + last_spread_index}
            )
          );
        }
      } else {
        if (last_spread_index - prev_spread_index != 1) {
          parts.emplace_back(
            VertexAdaptor<op_array>::create(
              std::vector<VertexPtr>{args.begin() + prev_spread_index + 1, args.begin() + last_spread_index}
            )
          );
        }
      }

      parts.emplace_back(args[i].try_as<op_spread>()->expr());

      prev_spread_index = last_spread_index;
    }
  }

  if (last_spread_index != args.size() - 1) {
    parts.emplace_back(
      VertexAdaptor<op_array>::create(
        std::vector<VertexPtr>{args.begin() + last_spread_index + 1, args.end()}
      )
    );
  }

  const auto array_merge_spread = G->get_function("array_merge_spread");
  if (!array_merge_spread) {
    return array_vertex;
  }

  auto call = VertexAdaptor<op_func_call>::create(parts);
  call->func_id = array_merge_spread;
  call->set_string(array_merge_spread->name);

  return call;
}
