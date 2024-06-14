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

static void write_function_decl(FunctionPtr function, CodeGenerator &W) {
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
}

static void write_function_definition(FunctionPtr function, std::unordered_set<VarPtr> *already_declared_vars, CodeGenerator &W) {
  stage::set_function(function);
  function->name_gen_map = {};  // make codegeneration of this function idempotent

  declare_vars_vector(function->global_var_ids, already_declared_vars, W);
  declare_vars_set(function->explicit_const_var_ids, already_declared_vars, W);
  declare_vars_vector(function->static_var_ids, already_declared_vars, W);
  W << UnlockComments();
  W << function->root << NL;
  W << LockComments();
}

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
  declare_vars_set(function->explicit_header_const_var_ids, nullptr, W);
  write_function_decl(function, W);
  if (function->is_inline) {
    W << CloseNamespace();
    includes.start_next_block();
    includes.add_function_body_depends(function);
    W << includes;
    W << OpenNamespace();
  }

  if (function->is_inline) {
    write_function_definition(function, nullptr, W);
  }

  W << CloseNamespace();
  W << CloseFile();
}

void FunctionListH::compile(CodeGenerator &W) const {
  W << OpenFile(functions[0]->header_name, functions[0]->subdir);
  W << "#pragma once" << NL << ExternInclude(G->settings().runtime_headers.get());

  IncludesCollector includes;
  bool has_inline = false;
  for (const auto &f : functions) {
    includes.add_function_signature_depends(f);
    has_inline = has_inline || f->is_inline;
  }
  W << includes;

  W << OpenNamespace();

  std::unordered_set<VarPtr> already_declared_vars;
  for (const auto &f : functions) {
    declare_vars_set(f->explicit_header_const_var_ids, &already_declared_vars, W);
  }

  for (const auto &f : functions) {
    write_function_decl(f, W);
  }

  if (has_inline) {
    W << CloseNamespace();
    includes.start_next_block();
    for (const auto &f : functions) {
      if (f->is_inline) {
        includes.add_function_body_depends(f);
      }
    }
    W << includes;
    W << OpenNamespace();
  }

  for (const auto &f : functions) {
    if (f->is_inline) {
      write_function_definition(f, &already_declared_vars, W);
    }
  }

  W << CloseNamespace();
  W << CloseFile();
}
