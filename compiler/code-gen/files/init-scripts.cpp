// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/init-scripts.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/shape-keys.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"

struct StaticInit {
  void compile(CodeGenerator &W) const;
};


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

    FunctionSignatureGenerator(W) << ("char **get_runtime_options([[maybe_unused]] int *count)") << BEGIN;
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

  const auto &ffi = G->get_ffi_root();
  const auto &ffi_shared_libs = ffi.get_shared_libs();
  if (!ffi_shared_libs.empty()) {
    W << "ffi_env_instance = FFIEnv{" << ffi_shared_libs.size() << ", " <<  ffi.get_dynamic_symbols_num() << "};" << NL;
    W << "ffi_env_instance.funcs.dlopen = dlopen;" << NL;
    W << "ffi_env_instance.funcs.dlsym = dlsym;" << NL;
    for (const auto &lib : ffi_shared_libs) {
      W << "ffi_env_instance.libs[" << lib.id << "].path = " << RawString{lib.path} << ";" << NL;
    }
    const auto &ffi_scopes = ffi.get_scopes();
    for (const auto &ffi_scope : ffi_scopes) {
      for (const auto &sym : ffi_scope->variables) {
        W << "ffi_env_instance.symbols[" << sym.env_index << "].name = \"" << sym.name() << "\";" << NL;
      }
      for (const auto &sym : ffi_scope->functions) {
        W << "ffi_env_instance.symbols[" << sym.env_index << "].name = \"" << sym.name() << "\";" << NL;
      }
    }
  }

  W << END << NL;
  W << CloseNamespace();
}

struct RunFunction {
  FunctionPtr function;
  RunFunction(FunctionPtr function) : function(function) {}

  void compile(CodeGenerator &W) const {
    FunctionSignatureGenerator(W) << "void " << FunctionName(function) << "$run() " << BEGIN
      << "TRY_CALL_VOID (void, " << FunctionName(function) << "());" << NL
      << "finish (0, false);" << NL
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


InitScriptsCpp::InitScriptsCpp(SrcFilePtr main_file_id) :
  main_file_id(main_file_id) {}

void InitScriptsCpp::compile(CodeGenerator &W) const {
  W << OpenFile("init_php_scripts.cpp", "", false);

  W << ExternInclude(G->settings().runtime_headers.get()) <<
    ExternInclude("server/php-init-scripts.h");

  W << Include(main_file_id->main_function->header_full_name);

  if (!G->get_ffi_root().get_shared_libs().empty()) {
    W << ExternInclude("runtime/ffi.h"); // FFI env singleton
    W << ExternInclude("dlfcn.h"); // dlopen, dlsym
  }

  if (!G->settings().is_static_lib_mode()) {
    W << NL;
    FunctionSignatureGenerator(W) << "void global_init_php_scripts()" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "void init_php_scripts()" << SemicolonAndNL();
  }

  W << NL << StaticInit() << NL;

  if (G->settings().is_static_lib_mode()) {
    W << LibGlobalVarsReset(main_file_id->main_function);
    W << CloseFile();
    return;
  }

  W << RunFunction(main_file_id->main_function) << NL;
  W << GlobalResetFunction(main_file_id->main_function) << NL;

  FunctionSignatureGenerator(W) << "void init_php_scripts() " << BEGIN;

  W << ShapeKeys::get_function_declaration() << ";" << NL;
  W << ShapeKeys::get_function_name() << "();" << NL << NL;

  W << FunctionName(main_file_id->main_function) << "$global_reset();" << NL;

  W << "set_script ("
    << FunctionName(main_file_id->main_function) << "$run, "
    << FunctionName(main_file_id->main_function) << "$global_reset);" << NL;

  W << END;

  W << CloseFile();
}

void LibVersionHFile::compile(CodeGenerator &W) const {
  W << OpenFile("_lib_version.h");
  W << "// Runtime sha256: " << G->settings().runtime_sha256.get() << NL;
  W << "// CXX: " << G->settings().cxx.get() << NL;
  W << "// CXXFLAGS DEFAULT: " << G->settings().cxx_flags_default.flags.get() << NL;
  W << "// CXXFLAGS WITH EXTRA: " << G->settings().cxx_flags_with_debug.flags.get() << NL;
  W << CloseFile();
}

void CppMainFile::compile(CodeGenerator &W) const {
  kphp_assert(G->settings().is_server_mode() || G->settings().is_cli_mode());
  W << OpenFile("main.cpp");
  W << ExternInclude("server/php-engine.h") << NL;

  W << "int main(int argc, char *argv[]) " << BEGIN
    << "return run_main(argc, argv, php_mode::" << G->settings().mode.get() << ")" << SemicolonAndNL{}
    << END;
  W << CloseFile();
}
