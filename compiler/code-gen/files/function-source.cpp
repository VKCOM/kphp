// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/function-source.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/stage.h"

FunctionCpp::FunctionCpp(FunctionPtr function) :
  function(function) {
}

void declare_global_vars(FunctionPtr function, CodeGenerator &W) {
  if (!function->global_var_ids.empty() || !function->static_var_ids.empty()) {
    W << "extern char *globals_linear_mem;" << NL;
  }
  // todo do something about builtin globals (declarations also)
  for (VarPtr var : function->global_var_ids) {
    if (var->is_builtin_global()) {
      W << VarExternDeclaration(var);
    }
  }
}

void declare_const_vars(FunctionPtr function, CodeGenerator &W) {
  if (!function->explicit_const_var_ids.empty()) {
    W << "extern char *constants_linear_mem;" << NL;
  }
}

void FunctionCpp::compile(CodeGenerator &W) const {
  if (function->is_inline) {
    return;
  }
  W << OpenFile(function->src_name, function->subdir);
  W << ExternInclude(G->settings().runtime_headers.get());
  W << Include(function->header_full_name);

  stage::set_function(function);
  function->name_gen_map = {};  // make codegeneration of this function idempotent

  IncludesCollector includes;
  includes.add_function_body_depends(function);
  W << includes;

  W << OpenNamespace();
  declare_global_vars(function, W);
  declare_const_vars(function, W);

  W << UnlockComments();
  W << function->root << NL;
  W << LockComments();

  W << CloseNamespace();
  W << CloseFile();
}
