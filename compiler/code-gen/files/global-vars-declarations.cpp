// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-declarations.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"

GlobalVarsDeclarations::GlobalVarsDeclarations(std::vector<VarPtr> &&all_mutable_globals)
  : all_mutable_globals(std::move(all_mutable_globals)) {}

void GlobalVarsDeclarations::compile(CodeGenerator &W) const {
  W << OpenFile("globals.builtin.cpp", "o_globals", false);

  W << ExternInclude(G->settings().runtime_headers.get());

  for (VarPtr var : all_mutable_globals) {
    if (var->is_builtin_global()) {
      W << VarDeclaration(var);
    }
  }

  W << CloseFile();
}
