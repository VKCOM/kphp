// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/function-source.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/stage.h"
#include <string>

FunctionCpp::FunctionCpp(FunctionPtr function) :
  function(function) {
}

void declare_const_vars(FunctionPtr function, CodeGenerator &W) {
  ConstantsExternCollector c_mem_extern;
  for (VarPtr var : function->explicit_const_var_ids) {
    c_mem_extern.add_extern_from_var(var);
  }
  W << c_mem_extern << NL;
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

  std::string name = function->name;
  bool good = name.find("test") != std::string::npos;
  if (good) {

    puts("YYYYYYYY");
  }

  IncludesCollector includes;
  includes.add_function_body_depends(function); // here

  W << includes;

  W << OpenNamespace();
  if (function->is_no_return) {
    W << "[[noreturn]] ";
  }
  declare_const_vars(function, W);

  W << UnlockComments();
  W << function->root << NL;
  W << LockComments();

  if (good) {
    static int here = 0;
    here++;
    printf("Attempt #%d\n", here);
    // TODO
    // understand why each var dump argument don't do anything during type infer
    // Or why type info is present during code gen, but isn\t during optimization pass
  }

  W << CloseNamespace();
  W << CloseFile();
}
