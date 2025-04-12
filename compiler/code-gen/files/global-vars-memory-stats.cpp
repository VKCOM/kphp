// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-memory-stats.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

GlobalVarsMemoryStats::GlobalVarsMemoryStats(const std::vector<VarPtr> &all_globals) {
  for (VarPtr global_var : all_globals) {
    bool is_primitive = vk::any_of_equal(tinf::get_type(global_var)->get_real_ptype(), tp_bool, tp_int, tp_float, tp_regexp, tp_any);
    if (!is_primitive && !global_var->is_builtin_runtime) {
      all_nonprimitive_globals.push_back(global_var);
    }
  }
  // to make codegen stable (here we use operator < of VarPtr, see var-data.cpp)
  std::sort(all_nonprimitive_globals.begin(), all_nonprimitive_globals.end());
}

void GlobalVarsMemoryStats::compile(CodeGenerator &W) const {
  int total_count = static_cast<int>(all_nonprimitive_globals.size());
  int parts_cnt = static_cast<int>(std::ceil(static_cast<double>(total_count) / N_GLOBALS_PER_FILE));

  W << OpenFile("globals_memory_stats.cpp", "", false)
    << ExternInclude(G->settings().runtime_headers.get())
    << OpenNamespace();

  // we don't take libs into account here (don't call "global memory stats" for every lib),
  // since we have to guarantee that libs were compiled with a necessary flag also
  // (most likely, not, then C++ compilation will fail)

  FunctionSignatureGenerator(W) << "array<int64_t> " << getter_name_ << "(int64_t lower_bound, "
                                << PhpMutableGlobalsConstRefArgument() << ")" << BEGIN
                                << "array<int64_t> result;" << NL
                                << "result.reserve(" << total_count << ", false);" << NL << NL;

  for (int part_id = 0; part_id < parts_cnt; ++part_id) {
    const std::string func_name_i = getter_name_ + std::to_string(part_id);
    // function declaration
    FunctionSignatureGenerator(W) << "void " << func_name_i << "(int64_t lower_bound, array<int64_t> &result, " << PhpMutableGlobalsConstRefArgument() << ")" << SemicolonAndNL();
    // function call
    W << func_name_i << "(lower_bound, result, php_globals);" << NL << NL;
  }

  W << "return result;" << NL << END
    << CloseNamespace()
    << CloseFile();

  for (int part_id = 0; part_id < parts_cnt; ++part_id) {
    int offset = part_id * N_GLOBALS_PER_FILE;
    int count = std::min(static_cast<int>(all_nonprimitive_globals.size()) - offset, N_GLOBALS_PER_FILE);

    W << OpenFile("globals_memory_stats." + std::to_string(part_id) + ".cpp", "o_globals_memory_stats", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_getter_part(W, part_id, all_nonprimitive_globals, offset, count);
    W << CloseFile();
  }
}

void GlobalVarsMemoryStats::compile_getter_part(CodeGenerator &W, int part_id, const std::vector<VarPtr> &global_vars, int offset, int count) {
  IncludesCollector includes;
  std::vector<std::string> var_names;
  var_names.reserve(count);
  for (int i = 0; i < count; ++i) {
    VarPtr global_var = global_vars[offset + i];
    includes.add_var_signature_depends(global_var);
    std::string var_name;
    if (global_var->is_function_static_var()) {
      var_name = global_var->holder_func->name + "::";
    }
    var_name.append(global_var->as_human_readable());
    var_names.emplace_back(std::move(var_name));
  }

  W << includes << NL
    << OpenNamespace();

  const auto var_name_shifts = compile_raw_data(W, var_names);
  W << NL;

  FunctionSignatureGenerator(W) << "static string get_raw_string(int raw_offset) " << BEGIN;
  W << "string str;" << NL
    << "str.assign_raw(&raw[raw_offset]);" << NL
    << "return str;" << NL
    << END << NL << NL;

  FunctionSignatureGenerator(W) << "void " << getter_name_ << part_id << "(int64_t lower_bound, array<int64_t> &result, "
                                << PhpMutableGlobalsConstRefArgument() << ")" << BEGIN;

  if (count) {
    W << "int64_t estimation = 0;" << NL;
  }
  for (int i = 0; i < count; ++i) {
    VarPtr global_var = global_vars[offset + i];
    W << "// " << var_names[i] << NL
      << "estimation = f$estimate_memory_usage(" << GlobalVarInPhpGlobals(global_var) << ");" << NL
      << "if (estimation > lower_bound) " << BEGIN
      << "result.set_value(get_raw_string(" << var_name_shifts[i] << "), estimation);" << NL
      << END << NL;
  }

  W << END;
  W << CloseNamespace();
}
