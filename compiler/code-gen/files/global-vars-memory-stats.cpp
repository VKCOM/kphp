// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-memory-stats.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/vars-collector.h"
#include "compiler/inferring/public.h"

GlobalVarsMemoryStats::GlobalVarsMemoryStats(SrcFilePtr main_file) :
  main_file_{main_file} {
}

void GlobalVarsMemoryStats::compile(CodeGenerator &W) const {
  VarsCollector vars_collector{32, [](VarPtr global_var) {
    return vk::none_of_equal(tinf::get_type(global_var)->get_real_ptype(), tp_bool, tp_int, tp_float, tp_any);
  }};

  vars_collector.collect_global_and_static_vars_from(main_file_->main_function);

  auto global_var_parts = vars_collector.flush();
  size_t global_vars_count = 0;
  for (const auto &global_vars : global_var_parts) {
    global_vars_count += global_vars.size();
  }

  W << OpenFile("globals_memory_stats.cpp", "", false)
    << ExternInclude(G->settings().runtime_headers.get())
    << OpenNamespace();

  // we don't take libs into account here (don't call "global memory stats" for every lib),
  // since we have to guarantee that libs were compiled with a necessary flag also
  // (most likely, not, then C++ compilation will fail)

  FunctionSignatureGenerator(W) << "array<int64_t> " << getter_name_ << "(int64_t lower_bound) " << BEGIN
                                << "array<int64_t> result;" << NL
                                << "result.reserve(" << global_vars_count << ", false);" << NL << NL;

  for (size_t part_id = 0; part_id < global_var_parts.size(); ++part_id) {
    const std::string func_name_i = getter_name_ + std::to_string(part_id);
    // function declaration
    FunctionSignatureGenerator(W) << "void " << func_name_i << "(int64_t lower_bound, array<int64_t> &result)" << SemicolonAndNL();
    // function call
    W << func_name_i << "(lower_bound, result);" << NL << NL;
  }

  W << "return result;" << NL << END
    << CloseNamespace()
    << CloseFile();

  for (size_t part_id = 0; part_id < global_var_parts.size(); ++part_id) {
    compile_getter_part(W, global_var_parts[part_id], part_id);
  }
}

void GlobalVarsMemoryStats::compile_getter_part(CodeGenerator &W, const std::set<VarPtr> &global_vars, size_t part_id) const {
  W << OpenFile("globals_memory_stats." + std::to_string(part_id) + ".cpp", "o_globals_memory_stats", false)
    << ExternInclude(G->settings().runtime_headers.get());

  IncludesCollector includes;
  std::vector<std::string> var_names;
  var_names.reserve(global_vars.size());
  for (const auto &global_var : global_vars) {
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

  FunctionSignatureGenerator(W) << "void " << getter_name_ << part_id << "(int64_t lower_bound, array<int64_t> &result) " << BEGIN
                                << "int64_t estimation = 0;" << NL;
  size_t var_num = 0;
  for (auto global_var : global_vars) {
    W << VarDeclaration(global_var, true, false)
      << "estimation = f$estimate_memory_usage(" << VarName(global_var) << ");" << NL
      << "if (estimation > lower_bound) " << BEGIN
      << "result.set_value(get_raw_string(" << var_name_shifts[var_num] << "), estimation);" << NL
      << END << NL;
    var_num++;
  }

  W << END;

  W << CloseNamespace()
    << CloseFile();
}
