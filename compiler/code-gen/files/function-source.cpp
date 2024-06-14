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

#include "common/algorithms/contains.h"

FunctionCpp::FunctionCpp(FunctionPtr function) :
  function(function) {
}

void declare_vars_vector(const std::vector<VarPtr> &vars, std::unordered_set<VarPtr> *already_declared, CodeGenerator &W) {
  for (auto v : vars) {
    if (already_declared != nullptr) {
      if (vk::contains(*already_declared, v)) {
        continue;
      }
      already_declared->insert(v);
    }
    W << VarExternDeclaration(v) << NL;
  }
}

void declare_vars_set(const std::set<VarPtr> &vars, std::unordered_set<VarPtr> *already_declared, CodeGenerator &W) {
  for (auto v : vars) {
    if (already_declared != nullptr) {
      if (vk::contains(*already_declared, v)) {
        continue;
      }
      already_declared->insert(v);
    }
    W << VarExternDeclaration(v) << NL;
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
  declare_vars_vector(function->global_var_ids, nullptr, W);
  declare_vars_set(function->explicit_const_var_ids, nullptr, W);
  declare_vars_vector(function->static_var_ids, nullptr, W);

  W << UnlockComments();
  W << function->root << NL;
  W << LockComments();

  W << CloseNamespace();
  W << CloseFile();
}

void FunctionListCpp::compile(CodeGenerator &W) const {
  FunctionPtr cpp_func;
  for (const auto &f : functions) {
    if (!f->is_inline) {
      cpp_func = f;
      break;
    }
  }
  if (!cpp_func) {
    return;
  }

  W << OpenFile(cpp_func->src_name, cpp_func->subdir);
  W << ExternInclude(G->settings().runtime_headers.get());
  W << Include(cpp_func->header_full_name);

  IncludesCollector includes;
  for (const auto &function : functions) {
    if (function->is_inline) {
      continue;
    }
    includes.add_function_body_depends(function);
  }
  W << includes;

  W << OpenNamespace();

  std::unordered_set<VarPtr> already_declared_vars;
  for (const auto &function : functions) {
    if (function->is_inline) {
      continue;
    }
    declare_vars_vector(function->global_var_ids, &already_declared_vars, W);
    declare_vars_set(function->explicit_const_var_ids, &already_declared_vars, W);
    declare_vars_vector(function->static_var_ids, &already_declared_vars, W);
  }

  for (const auto &function : functions) {
    if (function->is_inline) {
      continue;
    }

    stage::set_function(function);
    function->name_gen_map = {}; // make codegeneration of this function idempotent

    W << UnlockComments();
    W << function->root << NL;
    W << LockComments();
  }

  W << CloseNamespace();
  W << CloseFile();
}
