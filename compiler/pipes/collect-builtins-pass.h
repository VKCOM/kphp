// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class CollectBuiltinsPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Collect builtins pass";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (root->type() == op_func_call) {
      auto call = root.as<op_func_call>();
      if (!call->func_id) {
        call.debugPrint();
      }
      if (call->func_id->is_extern()) {
        auto fun_name = call->func_id->as_human_readable();
        {
          std::unique_lock guard(G->bu_mutex);
          G->builtin_usages[fun_name]++;
        }
      }
    }
    return root;
  }
};
