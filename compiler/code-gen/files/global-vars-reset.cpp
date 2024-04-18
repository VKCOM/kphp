// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-reset.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-linear-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

GlobalVarsReset::GlobalVarsReset(std::vector<std::vector<VarPtr>> &&all_globals_batched)
  : all_globals_batched(std::move(all_globals_batched)) {}

void GlobalVarsReset::compile_globals_reset_part(CodeGenerator &W, int batch_num, const std::vector<VarPtr> &cur_batch) {
  IncludesCollector includes;
  for (VarPtr var : cur_batch) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  W << NL;
  ConstantsLinearMemExternCollector c_mem_extern;
  for (VarPtr var : cur_batch) {
    if (var->init_val) {
      c_mem_extern.add_batch_num_from_init_val(var->init_val);
    }
  }
  W << c_mem_extern << NL;

  FunctionSignatureGenerator(W) << "void global_vars_reset_file" << batch_num << "(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;
  for (VarPtr var : cur_batch) {
    if (var->is_builtin_runtime) {  // they are manually reset in runtime sources
      continue;
    }

    // todo probably, inline hard_reset_var() body, since it uses new(&)?
    W << "// " << var->as_human_readable() << NL;
    W << "hard_reset_var(" << GlobalVarInPhpGlobals(var);
    if (var->init_val) {
      W << ", " << var->init_val;
    }
    W << ");" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_globals_reset(CodeGenerator &W, int n_batches) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void global_vars_reset(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;

  for (int batch_num = 0; batch_num < n_batches; batch_num++) {
    const std::string func_name_i = "global_vars_reset_file" + std::to_string(batch_num);
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

  W << GlobalsLinearMemAllocation();

  W << END << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile(CodeGenerator &W) const {
  int n_batches = static_cast<int>(all_globals_batched.size());

  for (int batch_num = 0; batch_num < n_batches; batch_num++) {
    W << OpenFile("globals_reset." + std::to_string(batch_num) + ".cpp", "o_globals_reset", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_globals_reset_part(W, batch_num, all_globals_batched[batch_num]);
    W << CloseFile();
  }

  W << OpenFile("globals_reset.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_reset(W, n_batches);
  W << CloseFile();

  W << OpenFile("globals_allocate.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_allocate(W);
  W << CloseFile();
}
