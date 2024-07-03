// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-reset.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/src-file.h"

GlobalVarsReset::GlobalVarsReset(const GlobalsBatchedMem &all_globals_in_mem)
  : all_globals_in_mem(all_globals_in_mem) {}

void GlobalVarsReset::compile_globals_reset_part(CodeGenerator &W, const GlobalsBatchedMem::OneBatchInfo &batch) {
  IncludesCollector includes;
  for (VarPtr var : batch.globals) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  W << NL;
  ConstantsExternCollector c_mem_extern;
  for (VarPtr var : batch.globals) {
    if (var->init_val) {
      c_mem_extern.add_extern_from_init_val(var->init_val);
    }
  }
  W << c_mem_extern << NL;

  FunctionSignatureGenerator(W) << "void global_vars_reset_file" << batch.batch_idx << "(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;
  for (VarPtr var : batch.globals) {
    if (var->is_builtin_runtime) {  // they are manually reset in runtime sources
      continue;
    }

    // todo probably, inline hard_reset_var() body, since it uses new(&)?
    W << "// " << var->as_human_readable() << NL;
    W << "hard_reset_var(" << GlobalVarInPhpGlobals(var);
    if (!var->init_val.try_lock()) {
      kphp_error(false, "Failed to lock when codegen hard_reset_var");
    }
    if (var->init_val) {
      W << ", " << var->init_val;
    }
    var->init_val.unlock();
    W << ");" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_globals_reset(CodeGenerator &W, const GlobalsBatchedMem &all_globals_in_mem) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void global_vars_reset(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;

  for (const auto &batch : all_globals_in_mem.get_batches()) {
    const std::string func_name_i = "global_vars_reset_file" + std::to_string(batch.batch_idx);
    // function declaration
    W << "void " << func_name_i << "(" << PhpMutableGlobalsRefArgument() << ");" << NL;
    // function call
    W << func_name_i << "(php_globals);" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_globals_allocate(CodeGenerator &W) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void global_vars_allocate(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;

  W << GlobalsMemAllocation();

  W << END << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile(CodeGenerator &W) const {
  for (const auto &batch : all_globals_in_mem.get_batches()) {
    W << OpenFile("globals_reset." + std::to_string(batch.batch_idx) + ".cpp", "o_globals_reset", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_globals_reset_part(W, batch);
    W << CloseFile();
  }

  W << OpenFile("globals_reset.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_reset(W, all_globals_in_mem);
  W << CloseFile();

  W << OpenFile("globals_allocate.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_allocate(W);
  W << CloseFile();
}
