#include "compiler/code-gen/files/vars-reset.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

GlobalVarsReset::GlobalVarsReset(SrcFilePtr main_file) :
  main_file_(main_file) {
}

void GlobalVarsReset::declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W) {
  if (v->type() == op_var) {
    VarPtr var = v->get_var_id();
    if (externed_vars.insert(var).second) {
      W << VarExternDeclaration(var);
    }
    return;
  }
  for (VertexPtr son : *v) {
    declare_extern_for_init_val(son, externed_vars, W);
  }
}

void GlobalVarsReset::compile_part(FunctionPtr func, const set<VarPtr> &used_vars, int part_i, CodeGenerator &W) {
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

  W << "void " << GlobalVarsResetFuncName(func, part_i) << " " << BEGIN;
  for (auto var : used_vars) {
    if (G->env().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << "hard_reset_var(" << VarName(var);
    //FIXME: brk and comments
    if (var->init_val) {
      W << UnlockComments();
      W << ", " << var->init_val;
      W << LockComments();
    }
    W << ");" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_func(FunctionPtr func, const std::set<FunctionPtr> &used_functions, int parts_n, CodeGenerator &W) {
  W << OpenNamespace();
  W << "void " << GlobalVarsResetFuncName(func) << " " << BEGIN;
  for (auto func : used_functions) {
    if (func->type == FunctionData::func_global) {
      W << "extern bool " << FunctionCallFlag(func) << ";" << NL;
      W << FunctionCallFlag(func) << " = false;" << NL;
    }
  }

  for (int i = 0; i < parts_n; i++) {
    W << "void " << GlobalVarsResetFuncName(func, i) << ";" << NL;
    W << GlobalVarsResetFuncName(func, i) << ";" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

template<class It>
void collect_vars(set<VarPtr> *used_vars, int used_vars_cnt, It begin, It end) {
  for (; begin != end; begin++) {
    VarPtr var_id = *begin;
    size_t var_hash;
    if (var_id->class_id) {
      var_hash = vk::std_hash(var_id->class_id->file_id->main_func_name);
    } else {
      var_hash = vk::std_hash(var_id->name);
    }
    int bucket = var_hash % used_vars_cnt;
    used_vars[bucket].insert(var_id);
  }
}

void GlobalVarsReset::collect_used_funcs_and_vars(
  FunctionPtr func, std::set<FunctionPtr> *visited_functions,
  std::set<VarPtr> *used_vars, int used_vars_cnt) {
  for (auto to : func->dep) {
    if (visited_functions->insert(to).second) {
      collect_used_funcs_and_vars(to, visited_functions, used_vars, used_vars_cnt);
    }
  }

  collect_vars(used_vars, used_vars_cnt, func->global_var_ids.begin(), func->global_var_ids.end());
  collect_vars(used_vars, used_vars_cnt, func->static_var_ids.begin(), func->static_var_ids.end());
}

void GlobalVarsReset::compile(CodeGenerator &W) const {
  FunctionPtr main_func = main_file_->main_function;

  std::set<FunctionPtr> used_functions;

  const int parts_n = 32;
  std::set<VarPtr> used_vars[parts_n];
  collect_used_funcs_and_vars(main_func, &used_functions, used_vars, parts_n);

  auto last_part = std::remove_if(std::begin(used_vars), std::end(used_vars),
                                  [](const std::set<VarPtr> &p) { return p.empty(); });
  auto non_empty_parts = std::distance(std::begin(used_vars), last_part);

  static const std::string vars_reset_src_prefix = "vars_reset.";
  std::vector<std::string> src_names(non_empty_parts);
  for (int i = 0; i < non_empty_parts; i++) {
    src_names[i] = vars_reset_src_prefix + std::to_string(i) + "." + main_func->src_name;
  }

  for (int i = 0; i < non_empty_parts; i++) {
    W << OpenFile(src_names[i], "o_vars_reset", false);
    W << ExternInclude("php_functions.h");
    compile_part(main_func, used_vars[i], i, W);
    W << CloseFile();
  }

  W << OpenFile(vars_reset_src_prefix + main_func->src_name, "", false);
  W << ExternInclude("php_functions.h");
  compile_func(main_func, used_functions, non_empty_parts, W);
  W << CloseFile();
}
