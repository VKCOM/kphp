// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/init-scripts.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"

struct StaticInit {
  const std::vector<FunctionPtr> &all_functions;
  explicit StaticInit(const std::vector<FunctionPtr> &all_functions);
  void compile(CodeGenerator &W) const;
};


StaticInit::StaticInit(const vector<FunctionPtr> &all_functions) :
  all_functions(all_functions) {
}

void StaticInit::compile(CodeGenerator &W) const {
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace());
      FunctionSignatureGenerator(W) << "void global_init_lib_scripts()" << SemicolonAndNL();
      W << CloseNamespace();
    }
  }

  W << OpenNamespace();
  FunctionSignatureGenerator(W)  << "void const_vars_init()" << SemicolonAndNL() << NL;

  FunctionSignatureGenerator(W) << "void tl_str_const_init()" << SemicolonAndNL();
  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "array<mixed> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base>)" << SemicolonAndNL();
    W << "extern array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
    FunctionSignatureGenerator(W) << "void fill_tl_storers_ht()" << SemicolonAndNL() << NL;
  }
  if (G->settings().is_static_lib_mode()) {
    FunctionSignatureGenerator(W) << "void global_init_lib_scripts() " << BEGIN;
  } else {
    FunctionSignatureGenerator(W) << ("const char *get_php_scripts_version()") << BEGIN
                                  << "return " << RawString(G->settings().php_code_version.get()) << ";" << NL
                                  << END << NL << NL;

    FunctionSignatureGenerator(W) << ("char **get_runtime_options(int *count)") << BEGIN;
    const auto &runtime_opts = G->get_kphp_runtime_opts();
    if (runtime_opts.empty()) {
      W << "return nullptr;" << NL;
    } else {
      W << "*count = " << runtime_opts.size() << ";" << NL;
      for (size_t i = 0; i != runtime_opts.size(); ++i) {
        W << "static char arg" << i << "[] = " << RawString{runtime_opts[i]} << ";" << NL;
      }
      W << "static char *argv[] = " << BEGIN;
      for (size_t i = 0; i != runtime_opts.size(); ++i) {
        W << "arg" << i << "," << NL;
      }
      W << END << ";" << NL
        << "return argv;" << NL;
    }
    W << END << NL << NL;

    FunctionSignatureGenerator(W) << ("void global_init_php_scripts() ") << BEGIN;
    for (LibPtr lib: G->get_libs()) {
      if (lib && !lib->is_raw_php()) {
        W << lib->lib_namespace() << "::global_init_lib_scripts();" << NL;
      }
    }
  }
  if (!G->settings().tl_schema_file.get().empty()) {
    W << "tl_str_const_init();" << NL;
    if (G->get_untyped_rpc_tl_used()) {
      W << "fill_tl_storers_ht();" << NL;
      W << "register_tl_storers_table_and_fetcher(gen$tl_storers_ht, &gen$tl_fetch_wrapper);" << NL;
    }
  }
  W << "const_vars_init();" << NL;

  W << END << NL;
  W << CloseNamespace();
}

struct RunFunction {
  FunctionPtr function;
  RunFunction(FunctionPtr function) : function(function) {}

  void compile(CodeGenerator &W) const {
    FunctionSignatureGenerator(W) << "void " << FunctionName(function) << "$run() " << BEGIN
      << "TRY_CALL_VOID (void, " << FunctionName(function) << "());" << NL
      << "finish (0, true);" << NL
      << END;
    W << NL;
  }
};


struct GlobalResetFunction {
  FunctionPtr function;
  GlobalResetFunction(FunctionPtr function);
  void compile(CodeGenerator &W) const;
};

GlobalResetFunction::GlobalResetFunction(FunctionPtr function) :
  function(function) {
}

void GlobalResetFunction::compile(CodeGenerator &W) const {
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace());
      FunctionSignatureGenerator(W) << "void lib_global_vars_reset()" << SemicolonAndNL();
      W << CloseNamespace();
    }
  }

  FunctionSignatureGenerator(W) << "void " << FunctionName(function) << "$global_reset() " << BEGIN;
  W << "void " << GlobalVarsResetFuncName(function) << ";" << NL;
  W << GlobalVarsResetFuncName(function) << ";" << NL;
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << lib->lib_namespace() << "::lib_global_vars_reset();" << NL;
    }
  }
  W << END << NL;
}


struct LibGlobalVarsReset {
  const FunctionPtr &main_function;
  LibGlobalVarsReset(const FunctionPtr &main_function);
  void compile(CodeGenerator &W) const;
};

LibGlobalVarsReset::LibGlobalVarsReset(const FunctionPtr &main_function) :
  main_function(main_function) {
}

void LibGlobalVarsReset::compile(CodeGenerator &W) const {
  W << OpenNamespace();
  FunctionSignatureGenerator(W) << "void lib_global_vars_reset() " << BEGIN
                                << "void " << GlobalVarsResetFuncName(main_function) << ";" << NL
                                << GlobalVarsResetFuncName(main_function) << ";" << NL
                                << END << NL << NL;
  W << "extern bool v$" << main_function->file_id->get_main_func_run_var_name() << ";" << NL;
  W << CloseNamespace();

  W << StaticLibraryRunGlobal(gen_out_style::cpp) << BEGIN
    << "using namespace " << G->get_global_namespace() << ";" << NL
    << "if (!v$" << main_function->file_id->get_main_func_run_var_name() << ")" << BEGIN
    << FunctionName(main_function) << "();" << NL
    << END << NL
    << END << NL << NL;
}


InitScriptsCpp::InitScriptsCpp(SrcFilePtr main_file_id, vector<FunctionPtr> &&all_functions) :
  main_file_id(main_file_id),
  all_functions(std::move(all_functions)) {}

void InitScriptsCpp::compile(CodeGenerator &W) const {
  W << OpenFile("init_php_scripts.cpp", "", false);

  W << ExternInclude("runtime-headers.h") <<
    ExternInclude("server/php-script.h");

  W << Include(main_file_id->main_function->header_full_name);

  if (!G->settings().is_static_lib_mode()) {
    W << NL;
    FunctionSignatureGenerator(W) << "void global_init_php_scripts()" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "void init_php_scripts()" << SemicolonAndNL();
  }

  W << NL << StaticInit(all_functions) << NL;

  if (G->settings().is_static_lib_mode()) {
    W << LibGlobalVarsReset(main_file_id->main_function);
    W << CloseFile();
    return;
  }

  W << RunFunction(main_file_id->main_function) << NL;
  W << GlobalResetFunction(main_file_id->main_function) << NL;

  FunctionSignatureGenerator(W) << "void init_php_scripts() " << BEGIN;

  W << FunctionName(main_file_id->main_function) << "$global_reset();" << NL;

  W << "set_script ("
    << FunctionName(main_file_id->main_function) << "$run, "
    << FunctionName(main_file_id->main_function) << "$global_reset);" << NL;

  W << END;

  W << CloseFile();
}
