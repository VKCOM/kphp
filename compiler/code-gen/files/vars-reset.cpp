// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/vars-reset.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/vars-collector.h"
#include "compiler/vertex.h"

GlobalVarsReset::GlobalVarsReset(SrcFilePtr main_file) :
  main_file_(main_file) {
}

void GlobalVarsReset::declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W) {
  if (auto var_vertex = v.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    if (externed_vars.insert(var).second) {
      W << VarExternDeclaration(var);
    }
    return;
  }
  for (VertexPtr son : *v) {
    declare_extern_for_init_val(son, externed_vars, W);
  }
}

void GlobalVarsReset::compile_part(FunctionPtr func, const std::set<VarPtr> &used_vars, int part_i, CodeGenerator &W) {
  IncludesCollector includes;
  for (auto var : used_vars) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  std::set<VarPtr> externed_vars;
  for (auto var : used_vars) {
    W << VarExternDeclaration(var);
    if (var->init_val) {
      declare_extern_for_init_val(var->init_val, externed_vars, W);
    }
  }

  FunctionSignatureGenerator(W) << "void " << GlobalVarsResetFuncName(func, part_i) << " " << BEGIN;
  for (auto var : used_vars) {
    if (G->settings().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << "hard_reset_var(" << VarName(var);
    //FIXME: brk and comments
    if (var->init_val) {
      W << ", " << var->init_val;
    }
    W << ");" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_func(FunctionPtr func, int parts_n, CodeGenerator &W) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void " << GlobalVarsResetFuncName(func) << " " << BEGIN;

  for (int i = 0; i < parts_n; i++) {
    W << "void " << GlobalVarsResetFuncName(func, i) << ";" << NL;
    W << GlobalVarsResetFuncName(func, i) << ";" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile(CodeGenerator &W) const {
  FunctionPtr main_func = main_file_->main_function;

  VarsCollector vars_collector{32};
  vars_collector.collect_global_and_static_vars_from(main_func);
  auto used_vars = vars_collector.flush();

  static const std::string vars_reset_src_prefix = "vars_reset.";
  std::vector<std::string> src_names(used_vars.size());
  for (int i = 0; i < used_vars.size(); i++) {
    src_names[i] = vars_reset_src_prefix + std::to_string(i) + "." + main_func->src_name;
  }

  for (int i = 0; i < used_vars.size(); i++) {
    W << OpenFile(src_names[i], "o_vars_reset", false);
    W << ExternInclude("runtime-headers.h");
    compile_part(main_func, used_vars[i], i, W);
    W << CloseFile();
  }

  W << OpenFile(vars_reset_src_prefix + main_func->src_name, "", false);
  W << ExternInclude("runtime-headers.h");
  compile_func(main_func, used_vars.size(), W);
  W << CloseFile();
}
