// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-declarations.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"

GlobalVarsDeclarationsPart::GlobalVarsDeclarationsPart(std::vector<VarPtr> &&vars_of_part, size_t part_id)
  : vars_of_part_(std::move(vars_of_part))
  , part_id(part_id) {
  std::sort(vars_of_part_.begin(), vars_of_part_.end());
}

void GlobalVarsDeclarationsPart::compile(CodeGenerator &W) const {
  W << OpenFile("globals." + std::to_string(part_id) + ".cpp", "o_globals", false);

  W << ExternInclude(G->settings().runtime_headers.get());

  IncludesCollector includes;
  for (VarPtr var : vars_of_part_) {
    if (G->settings().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    includes.add_var_signature_depends(var);
    includes.add_vertex_depends(var->init_val);
  }
  W << includes;

  W << OpenNamespace();
  for (VarPtr var : vars_of_part_) {
    if (G->settings().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << VarDeclaration(var);
  }

  W << CloseNamespace();
  W << CloseFile();
}
