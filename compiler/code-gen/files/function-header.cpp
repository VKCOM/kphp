// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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
  W << "#pragma once" << NL << ExternInclude(G->settings().runtime_headers.get());

  IncludesCollector includes;
  includes.add_function_signature_depends(function);
  W << includes;

  W << OpenNamespace();
  if (!function->explicit_header_const_var_ids.empty()) {   // default value of function argument is const var
    W << ConstantsLinearMemDeclaration(true) << NL;
  }

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

    declare_global_vars(function, W);
    declare_const_vars(function, W);
    W << UnlockComments();
    W << function->root << NL;
    W << LockComments();
  }

  W << CloseNamespace();
  W << CloseFile();
}
