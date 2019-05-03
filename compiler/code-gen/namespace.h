#pragma once

#include "compiler/code-gen/common.h"
#include "compiler/compiler-core.h"

struct OpenNamespace {
  OpenNamespace() : OpenNamespace(G->get_global_namespace()) { }

  explicit OpenNamespace(const string &ns) : ns_(ns) {}

  void compile(CodeGenerator &W) const {
    if (!ns_.empty()) {
      kphp_assert(!W.get_context().namespace_opened);
      W << "namespace " << ns_ << " {" << NL;
      W.get_context().namespace_opened = true;
    }
  }

private:
  const string &ns_;
};

struct CloseNamespace {
  void compile(CodeGenerator &W) const {
    if (W.get_context().namespace_opened) {
      W << "}" << NL << NL;
      W.get_context().namespace_opened = false;
    }
  }
};
