#include "compiler/code-gen/files/function-header.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/function-source.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/function-data.h"

FunctionH::FunctionH(FunctionPtr function) :
  function(function) {
}

void FunctionH::compile(CodeGenerator &W) const {
  W << OpenFile(function->header_name, function->subdir);
  W << "#pragma once" << NL << ExternInclude("php_functions.h");

  IncludesCollector includes;
  includes.add_function_signature_depends(function);
  W << includes;

  W << OpenNamespace();
  for (auto const_var : function->explicit_header_const_var_ids) {
    W << VarExternDeclaration(const_var) << NL;
  }

  if (function->type == FunctionData::func_global) {
    W << "extern bool " << FunctionCallFlag(function) << ";" << NL;
  }

  if (function->is_inline) {
    W << "static inline ";
  }
  W << FunctionDeclaration(function, true) << ";" << NL;
  if (function->is_resumable) {
    W << FunctionForkDeclaration(function, true) << ";" << NL;
  }
  if (function->is_inline) {
    stage::set_function(function);
    declare_global_vars(function, W);
    declare_const_vars(function, W);
    declare_static_vars(function, W);
    W << CloseNamespace();
    includes.start_next_block();
    includes.add_function_body_depends(function);
    W << includes;
    W << OpenNamespace();
    W << UnlockComments();
    W << function->root << NL;
    W << LockComments();
  }

  W << CloseNamespace();
  W << CloseFile();
}
