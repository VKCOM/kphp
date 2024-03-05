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
#include "compiler/inferring/public.h"

struct GlobalInLinearMem {  // todo duplicate
  VarPtr global_var;

  explicit GlobalInLinearMem(VarPtr mutable_global_var)
    : global_var(mutable_global_var) {}

  void compile(CodeGenerator &W) const {
    kphp_assert(global_var->offset_in_linear_mem >= 0);
    W << "(*reinterpret_cast<" << type_out(tinf::get_type(global_var)) << "*>(globals_linear_mem+" << global_var->offset_in_linear_mem << "))";
  }
};

GlobalVarsReset::GlobalVarsReset(std::vector<VarPtr> &&all_globals) :
  all_globals(std::move(all_globals)) {
}

void GlobalVarsReset::compile_globals_reset_part(const std::vector<VarPtr> &used_vars, int part_id, CodeGenerator &W) {
  IncludesCollector includes;
  for (VarPtr var : used_vars) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  W << NL;
  W << "extern char *constants_linear_mem;" << NL;
  W << "extern char *globals_linear_mem;" << NL << NL;

  FunctionSignatureGenerator(W) << "void global_vars_reset_file" << part_id << "()" << BEGIN;
  for (VarPtr var : used_vars) {
    if (G->settings().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    // todo probably, output /* $var_name */ in GlobalInLinearMem always, don't forget about function statics
    W << "// " << var->as_human_readable() << NL;
    W << "hard_reset_var(" << GlobalInLinearMem(var);
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
  // todo check in vkcom, how many globals exist in all_globals, not not used (not found by VarsCollector)
  // todo if a small amount, then get rid on VarsCollector (it's also used for globals_memory_stats)
  VarsCollector vars_collector{32};
  vars_collector.collect_global_and_static_vars_from(G->get_main_file()->main_function);
  auto used_vars = vars_collector.flush();

  for (int i = 0; i < 1; i++) {
    W << OpenFile("globals_reset." + std::to_string(i) + ".cpp", "o_globals_reset", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_globals_reset_part(all_globals, i, W);
    W << CloseFile();
  }

  W << OpenFile("globals_reset.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_reset(1, W);
  W << CloseFile();
}
