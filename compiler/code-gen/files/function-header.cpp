// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/function-header.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
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
  W << "#pragma once" << NL << ExternInclude(G->settings().runtime_headers.get());

  IncludesCollector includes;
  includes.add_function_signature_depends(function);
  W << includes;

  W << OpenNamespace();

  ConstantsExternCollector c_mem_extern;
  for (VarPtr var : function->explicit_header_const_var_ids) {
    c_mem_extern.add_extern_from_var(var);
  }
  W << c_mem_extern << NL;

  if (function->is_inline) {
    W << "inline ";
  }
  W << FunctionDeclaration(function, true);
  if (function->is_no_return) {
    W << " __attribute__((noreturn))";
  }
  if (function->is_flatten) {
    W << " __attribute__((flatten))";
  }
  W << ";" << NL;
  if (function->is_resumable) {
    W << FunctionForkDeclaration(function, true) << ";" << NL;
  }
  if (function->is_inline) {
    stage::set_function(function);
    function->name_gen_map = {};  // make codegeneration of this function idempotent

    W << CloseNamespace();
    includes.start_next_block();
    includes.add_function_body_depends(function);
    W << includes;
    W << OpenNamespace();

    declare_const_vars(function, W);
    W << UnlockComments();
    W << function->root << NL;
    W << LockComments();
  }

  W << CloseNamespace();
  W << CloseFile();
}
