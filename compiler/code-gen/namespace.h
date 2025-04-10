// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/code-gen/common.h"
#include "compiler/compiler-core.h"

struct OpenNamespace {
  OpenNamespace()
      : OpenNamespace(G->get_global_namespace()) {}

  explicit OpenNamespace(const std::string& ns)
      : ns_(ns) {}

  void compile(CodeGenerator& W) const {
    if (!ns_.empty()) {
      kphp_assert(!W.get_context().namespace_opened);
      W << "namespace " << ns_ << " {" << NL;
      W.get_context().namespace_opened = true;
    }
  }

private:
  const std::string& ns_;
};

struct CloseNamespace {
  void compile(CodeGenerator& W) const {
    if (W.get_context().namespace_opened) {
      W << "}" << NL << NL;
      W.get_context().namespace_opened = false;
    }
  }
};
