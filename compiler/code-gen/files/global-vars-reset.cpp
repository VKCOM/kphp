// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-reset.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/src-file.h"
#include "compiler/data/vars-collector.h"

GlobalVarsReset::GlobalVarsReset(SrcFilePtr main_file) :
  main_file_(main_file) {
}

void GlobalVarsReset::declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W) {
  if (auto var_vertex = v.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    if (externed_vars.insert(var).second) {
      // todo avoid duplicates
      W << "extern char *constants_linear_mem;" << NL;
    }
    return;
  }
  for (VertexPtr son : *v) {
    declare_extern_for_init_val(son, externed_vars, W);
  }
}

void GlobalVarsReset::compile_globals_reset_part(const std::set<VarPtr> &used_vars, int part_i, CodeGenerator &W) {
  IncludesCollector includes;
  for (VarPtr var : used_vars) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  std::set<VarPtr> externed_vars;
  for (VarPtr var : used_vars) {
    W << VarExternDeclaration(var);
    if (var->init_val) {
      declare_extern_for_init_val(var->init_val, externed_vars, W);
    }
  }

  FunctionSignatureGenerator(W) << "void global_vars_reset_file" << part_i << "()" << BEGIN;
  for (VarPtr var : used_vars) {
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

void GlobalVarsReset::compile_globals_reset(int parts_n, CodeGenerator &W) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void global_vars_reset()" << BEGIN;

  for (int part_id = 0; part_id < parts_n; part_id++) {
    const std::string func_name_i = "global_vars_reset_file" + std::to_string(part_id);
    // function declaration
    W << "void " << func_name_i << "();" << NL;
    // function call
    W << func_name_i << "();" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile(CodeGenerator &W) const {
  VarsCollector vars_collector{32};
  vars_collector.collect_global_and_static_vars_from(main_file_->main_function);
  auto used_vars = vars_collector.flush();

  for (int i = 0; i < used_vars.size(); i++) {
    W << OpenFile("globals_reset." + std::to_string(i) + ".cpp", "o_globals_reset", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_globals_reset_part(used_vars[i], i, W);
    W << CloseFile();
  }

  W << OpenFile("globals_reset.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_reset(used_vars.size(), W);
  W << CloseFile();
}
