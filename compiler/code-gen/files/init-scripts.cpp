// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/init-scripts.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
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
  if (G->is_output_mode_lib()) {
    return;
  }

  // "const vars init" declarations
  FunctionSignatureGenerator(W) << "void const_vars_init()" << SemicolonAndNL() << NL;
  for (LibPtr lib : G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace());
      FunctionSignatureGenerator(W) << "void const_vars_init()" << SemicolonAndNL();
      W << CloseNamespace();
    }
  }

  FunctionSignatureGenerator(W) << "void tl_str_const_init()" << SemicolonAndNL();
  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "array<mixed> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base>)" << SemicolonAndNL();
    W << "extern array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
    FunctionSignatureGenerator(W) << "void fill_tl_storers_ht()" << SemicolonAndNL() << NL;
  }
  if (!G->is_output_mode_k2_component()) {
    FunctionSignatureGenerator(W) << ("const char *get_php_scripts_version()") << BEGIN << "return " << RawString(G->settings().php_code_version.get()) << ";"
                                  << NL << END << NL << NL;
  }

  if (!G->is_output_mode_k2_component()) {
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
      W << END << ";" << NL << "return argv;" << NL;
    }
    W << END << NL << NL;
  }

  FunctionSignatureGenerator(W) << ("void init_php_scripts_once_in_master() ") << BEGIN;

  if (!G->settings().tl_schema_file.get().empty()) {
    W << "tl_str_const_init();" << NL;
    if (G->get_untyped_rpc_tl_used()) {
      W << "fill_tl_storers_ht();" << NL;
      W << "register_tl_storers_table_and_fetcher(gen$tl_storers_ht, &gen$tl_fetch_wrapper);" << NL;
    }
  }
  W << "const_vars_init();" << NL;
  for (LibPtr lib : G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << lib->lib_namespace() << "::const_vars_init();" << NL;
    }
  }
  W << NL;
  FunctionSignatureGenerator(W) << "void " << ShapeKeys::get_function_name() << "()" << SemicolonAndNL();
  W << ShapeKeys::get_function_name() << "();" << NL;

  const auto &ffi = G->get_ffi_root();
  const auto &ffi_shared_libs = ffi.get_shared_libs();
  if (!ffi_shared_libs.empty()) {
    W << "ffi_env_instance = FFIEnv{" << ffi_shared_libs.size() << ", " << ffi.get_dynamic_symbols_num() << "};" << NL;
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
}

struct RunInterruptedFunction {
  FunctionPtr function;
  RunInterruptedFunction(FunctionPtr function) : function(function) {}

  void compile(CodeGenerator &W) const {
    std::string await_prefix = function->is_interruptible ? "co_await " : "";
    FunctionSignatureGenerator(W) << "task_t<void> " << FunctionName(function) << "$run() " << BEGIN
                                  << await_prefix << FunctionName(function) << "();" << NL
                                  << "co_return;"
                                  << END;
    W << NL;
  }
};

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


struct GlobalsResetFunction {
  FunctionPtr main_function;
  explicit GlobalsResetFunction(FunctionPtr main_function);
  void compile(CodeGenerator &W) const;
};

GlobalsResetFunction::GlobalsResetFunction(FunctionPtr main_function)
  : main_function(main_function) {}

void GlobalsResetFunction::compile(CodeGenerator &W) const {
  // "global vars reset" declarations
  FunctionSignatureGenerator(W) << "void global_vars_allocate(" << PhpMutableGlobalsRefArgument() << ")" << SemicolonAndNL();
  FunctionSignatureGenerator(W) << "void global_vars_reset(" << PhpMutableGlobalsRefArgument() << ")" << SemicolonAndNL();
  W << NL;
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace());
      FunctionSignatureGenerator(W) << "void global_vars_allocate(" << PhpMutableGlobalsRefArgument() << ")" << SemicolonAndNL();
      FunctionSignatureGenerator(W) << "void global_vars_reset(" << PhpMutableGlobalsRefArgument() << ")" << SemicolonAndNL();
      W << CloseNamespace();
    }
  }

  // "global vars reset" calls
  FunctionSignatureGenerator(W) << "void " << FunctionName(main_function) << "$globals_reset(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;
  W << "global_vars_reset(php_globals);" << NL;
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << lib->lib_namespace() << "::global_vars_reset(php_globals);" << NL;
    }
  }
  W << END << NL;
}


struct LibRunFunction {
  FunctionPtr main_function;
  LibRunFunction(FunctionPtr main_function);
  void compile(CodeGenerator &W) const;
};

LibRunFunction::LibRunFunction(FunctionPtr main_function) :
  main_function(main_function) {
}

void LibRunFunction::compile(CodeGenerator &W) const {
  // "run" functions just calls the main file of a lib
  // it's guaranteed that it doesn't contain code except declarations (body_value is empty),
  // that's why we shouldn't deal with `if (!called)` global
  W << StaticLibraryRunGlobal(gen_out_style::cpp) << BEGIN
    << "using namespace " << G->get_global_namespace() << ";" << NL
    << FunctionName(main_function) << "();" << NL
    << END << NL << NL;
}


InitScriptsCpp::InitScriptsCpp(SrcFilePtr main_file_id) :
  main_file_id(main_file_id) {}

void InitScriptsCpp::compile(CodeGenerator &W) const {
  W << OpenFile("init_php_scripts.cpp", "", false);

  W << ExternInclude(G->settings().runtime_headers.get());
  if (!G->is_output_mode_k2_component()) {
     W << ExternInclude("server/php-init-scripts.h");
  }


  W << Include(main_file_id->main_function->header_full_name);

  if (!G->get_ffi_root().get_shared_libs().empty()) {
    W << ExternInclude("runtime/ffi.h"); // FFI env singleton
    W << ExternInclude("dlfcn.h"); // dlopen, dlsym
  }

  W << NL << StaticInit() << NL;

  if (G->is_output_mode_lib()) {
    W << LibRunFunction(main_file_id->main_function);
    W << CloseFile();
    return;
  }

  if (G->is_output_mode_k2_component()) {
    W << RunInterruptedFunction(main_file_id->main_function) << NL;
  } else {
    W << RunFunction(main_file_id->main_function) << NL;
  }
  W << GlobalsResetFunction(main_file_id->main_function) << NL;

  if (G->is_output_mode_k2_component()) {
    FunctionSignatureGenerator(W) << "void init_php_scripts_in_each_worker(" << PhpMutableGlobalsRefArgument() << ", task_t<void> &run" ")" << BEGIN;
  } else {
    FunctionSignatureGenerator(W) << "void init_php_scripts_in_each_worker(" << PhpMutableGlobalsRefArgument() << ")" << BEGIN;
  }

  W << "global_vars_allocate(php_globals);" << NL;
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << lib->lib_namespace() << "::global_vars_allocate(php_globals);" << NL;
    }
  }

  W << FunctionName(main_file_id->main_function) << "$globals_reset(php_globals);" << NL;

  if (G->is_output_mode_k2_component()) {
    W << "run = " << FunctionName(main_file_id->main_function) << "$run();" << NL;
  } else {
    W << "set_script ("
      << FunctionName(main_file_id->main_function) << "$run, "
      << FunctionName(main_file_id->main_function) << "$globals_reset);" << NL;
  }

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
  kphp_assert(G->is_output_mode_server() || G->is_output_mode_cli());
  W << OpenFile("main.cpp");
  W << ExternInclude("server/php-engine.h") << NL;

  W << "int main(int argc, char *argv[]) " << BEGIN
    << "return run_main(argc, argv, php_mode::" << G->settings().mode.get() << ")" << SemicolonAndNL{}
    << END;
  W << CloseFile();
}

void ComponentInfoFile::compile(CodeGenerator &W) const {
  kphp_assert(G->is_output_mode_k2_component());
  G->settings().get_version();
  auto now = std::chrono::system_clock::now();
  W << OpenFile("image_info.cpp");
  W << ExternInclude(G->settings().runtime_headers.get());
  W << "const ImageInfo *vk_k2_describe() " << BEGIN
    << "static ImageInfo imageInfo {\"" << G->settings().k2_component_name.get() << "\"" << ","
                                        << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() << ","
                                        << "K2_PLATFORM_HEADER_H_VERSION, "
                                        << "{}," //todo:k2 add commit hash
                                        << "{}," //todo:k2 add compiler hash?
                                        << (G->settings().k2_component_is_oneshot.get() ? "1" : "0")
                                        << "};" << NL
    << "return &imageInfo;" << NL
    << END;
  W << CloseFile();
}
