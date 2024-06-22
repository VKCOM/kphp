
#pragma once

#include <string>

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

class ForceSyncPass final : public FunctionPassBase {
  private:
    int inside_force_sync = 0;

  public:
    std::string get_description() override {
      return "Process force_sync() calls";
    }

    VertexPtr on_enter_vertex(VertexPtr v) override;

    bool user_recursion(VertexPtr v) override;

};

