#include "compiler/code-gen/files/init-scripts.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
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
      W << OpenNamespace(lib->lib_namespace())
        << "void global_init_lib_scripts();" << NL
        << CloseNamespace();
    }
  }

  W << OpenNamespace();
  W << "void const_vars_init();" << NL << NL;

  W << "void tl_str_const_init();" << NL;
  W << "array<var> gen$tl_fetch_wrapper(unique_object<tl_func_base>);" << NL;
  W << "extern array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
  W << "void fill_tl_storers_ht();" << NL << NL;
  if (G->env().is_static_lib_mode()) {
    W << "void global_init_lib_scripts() " << BEGIN;
  } else {
    W << "void global_init_php_scripts() " << BEGIN;
    for (LibPtr lib: G->get_libs()) {
      if (lib && !lib->is_raw_php()) {
        W << lib->lib_namespace() << "::global_init_lib_scripts();" << NL;
      }
    }
  }
  if (!G->env().get_tl_schema_file().empty()) {
    W << "tl_str_const_init();" << NL;
    W << "fill_tl_storers_ht();" << NL;
    W << "register_tl_storers_table_and_fetcher(gen$tl_storers_ht, &gen$tl_fetch_wrapper);" << NL;
  }
  W << "const_vars_init();" << NL;

  W << END << NL;
  W << CloseNamespace();
}

struct RunFunction {
  FunctionPtr function;
  RunFunction(FunctionPtr function) : function(function) {}

  void compile(CodeGenerator &W) const {
    W << "void " << FunctionName(function) << "$run() " << BEGIN
      << "TRY_CALL_VOID (void, " << FunctionName(function) << "());" << NL
      << "finish (0);" << NL
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
      W << OpenNamespace(lib->lib_namespace())
        << "void lib_global_vars_reset();" << NL
        << CloseNamespace();
    }
  }

  W << "void " << FunctionName(function) << "$global_reset() " << BEGIN;
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
  W << "void lib_global_vars_reset() " << BEGIN
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


InitScriptsCpp::InitScriptsCpp(vector<SrcFilePtr> &&main_file_ids, vector<FunctionPtr> &&all_functions) :
  main_file_ids(std::move(main_file_ids)),
  all_functions(std::move(all_functions)) {}

void InitScriptsCpp::compile(CodeGenerator &W) const {
  W << OpenFile("init_php_scripts.cpp", "", false);

  W << ExternInclude("php_functions.h") <<
    ExternInclude("php_script.h");

  for (auto i : main_file_ids) {
    W << Include(i->main_function->header_full_name);
  }

  if (!G->env().is_static_lib_mode()) {
    W << NL
      << "void global_init_php_scripts();" << NL
      << "void init_php_scripts();" << NL;
  }

  W << NL << StaticInit(all_functions) << NL;

  if (G->env().is_static_lib_mode()) {
    // only one main file is allowed for static lib mode
    kphp_assert(main_file_ids.size() == 1);
    W << LibGlobalVarsReset(main_file_ids.back()->main_function);
    W << CloseFile();
    return;
  }

  for (auto i : main_file_ids) {
    W << RunFunction(i->main_function) << NL;
    W << GlobalResetFunction(i->main_function) << NL;
  }

  W << "void init_php_scripts() " << BEGIN;

  for (auto i : main_file_ids) {
    W << FunctionName(i->main_function) << "$global_reset();" << NL;

    W << "set_script (" <<
      "\"@" << i->short_file_name << "\", " <<
      FunctionName(i->main_function) << "$run, " <<
      FunctionName(i->main_function) << "$global_reset);" << NL;
  }

  W << END;

  W << CloseFile();
}
