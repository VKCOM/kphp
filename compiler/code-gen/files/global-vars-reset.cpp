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

GlobalVarsReset::GlobalVarsReset(std::vector<VarPtr> &&all_globals, int count_per_part)
  : all_globals(std::move(all_globals))
  , count_per_part(count_per_part) {}

void GlobalVarsReset::compile_globals_reset_part(CodeGenerator &W, int part_id, const std::vector<VarPtr> &all_globals, int offset, int count) {
  IncludesCollector includes;
  for (int i = 0; i < count; ++i) {
    includes.add_var_signature_depends(all_globals[offset + i]);
  }
  W << includes;
  W << OpenNamespace();

  W << NL;
  W << ConstantsLinearMemDeclaration(true) << NL;

  FunctionSignatureGenerator(W) << "void global_vars_reset_file" << part_id << "(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;
  for (int i = 0; i < count; ++i) {
    VarPtr var = all_globals[offset + i];
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

void GlobalVarsReset::compile_globals_reset(CodeGenerator &W, int parts_cnt) {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void global_vars_reset(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;

  for (int part_id = 0; part_id < parts_cnt; part_id++) {
    const std::string func_name_i = "global_vars_reset_file" + std::to_string(part_id);
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
  int parts_cnt = static_cast<int>(std::ceil(static_cast<double>(all_globals.size()) / count_per_part));

  for (int part_id = 0; part_id < parts_cnt; part_id++) {
    int offset = part_id * count_per_part;
    int count = std::min(static_cast<int>(all_globals.size()) - offset, count_per_part);

    W << OpenFile("globals_reset." + std::to_string(part_id) + ".cpp", "o_globals_reset", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_globals_reset_part(W, part_id, all_globals, offset, count);
    W << CloseFile();
  }

  W << OpenFile("globals_reset.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_reset(W, parts_cnt);
  W << CloseFile();

  W << OpenFile("globals_allocate.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_globals_allocate(W);
  W << CloseFile();
}
