// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/stats.h"

#include "common/dl-utils-lite.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"

void Stats::on_var_inserting(VarData::Type type) {
  switch (type) {
    case VarData::var_local_t:
      ++local_vars_;
      break;
    case VarData::var_local_inplace_t:
      ++local_inplace_vars_;
      break;
    case VarData::var_global_t:
      ++global_vars_;
      break;
    case VarData::var_param_t:
      ++param_vars_;
      break;
    case VarData::var_const_t:
      ++global_const_vars_;
      break;
    case VarData::var_static_t:
      ++static_vars_;
      break;
    case VarData::var_instance_t:
      ++instance_vars_;
      break;
    default:
      break;
  }
}

void Stats::on_function_processed(FunctionPtr function) {
  ++total_functions_;
  if (function->can_throw()) {
    ++total_throwing_functions_;
  }
  if (function->is_resumable) {
    ++total_resumable_functions_;
  }
  if (function->is_interruptible) {
    ++total_interruptible_functions_;
  }
  if (function->is_inline) {
    ++total_inline_functions_;
  }
}

void Stats::update_memory_stats() {
  const mem_info_t mem_info = get_self_mem_stats();
  memory_rss_ = mem_info.rss;
  memory_rss_peak_ = mem_info.rss_peak;
}

void Stats::write_to(std::ostream &out, bool with_indent) const {
  const char *indent = with_indent ? "  " : "";
  const char *block_sep = with_indent ? "\n" : "";
  out << indent << "classes.total: " << total_classes << std::endl;
  out << indent << "classes.total_lambdas: " << total_lambdas << std::endl;
  out << block_sep;
  out << indent << "vars.local: " << local_vars_ << std::endl;
  out << indent << "vars.local_inplace: " << local_inplace_vars_ << std::endl;
  out << indent << "vars.static: " << static_vars_ << std::endl;
  out << indent << "vars.global: " << global_vars_ << std::endl;
  out << indent << "vars.global_const: " << global_const_vars_ << std::endl;
  out << indent << "vars.param: " << param_vars_ << std::endl;
  out << indent << "vars.param_make_clone: " << cnt_make_clone << std::endl;
  out << block_sep;
  out << indent << "types.instance: " << instance_vars_ << std::endl;
  out << indent << "types.local_mixed: " << cnt_mixed_vars << std::endl;
  out << indent << "types.params_mixed: " << cnt_mixed_params << std::endl;
  out << indent << "types.const_params_mixed: " << cnt_const_mixed_params << std::endl;
  out << block_sep;
  out << indent << "functions.total: " << total_functions_ << std::endl;
  out << indent << "functions.total_inline: " << total_inline_functions_ << std::endl;
  out << indent << "functions.total_throwing: " << total_throwing_functions_ << std::endl;
  out << indent << "functions.total_resumable: " << total_resumable_functions_ << std::endl;
  out << indent << "functions.total_interruptible: " << total_interruptible_functions_ << std::endl;
  out << block_sep;
  out << indent << "memory.rss: " << memory_rss_ * 1024 << std::endl;
  out << indent << "memory.rss_peak: " << memory_rss_peak_ * 1024 << std::endl;
  out << block_sep;
  out << indent << "compilation.transpilation_time: " << transpilation_time << std::endl;
  out << indent << "compilation.total_time: " << total_time << std::endl;
  out << indent << "compilation.object_out_size: " << object_out_size << std::endl;
  out << block_sep;
  out << indent << "hash_tables.max_files: " << max_files << std::endl;
  out << indent << "hash_tables.total_files: " << total_files << std::endl;
  out << indent << "hash_tables.max_dirs: " << max_dirs << std::endl;
  out << indent << "hash_tables.total_dirs: " << total_dirs << std::endl;
  out << indent << "hash_tables.max_functions: " << max_functions << std::endl;
  out << indent << "hash_tables.total_functions: " << total_functions_ << std::endl;
  out << indent << "hash_tables.max_classes: " << max_classes << std::endl;
  out << indent << "hash_tables.total_classes: " << total_classes + total_lambdas << std::endl;
  out << indent << "hash_tables.max_defines: " << max_defines << std::endl;
  out << indent << "hash_tables.total_defines: " << total_defines << std::endl;
  out << indent << "hash_tables.max_constants: " << max_constants << std::endl;
  out << indent << "hash_tables.total_constants: " << global_const_vars_ << std::endl;
  out << indent << "hash_tables.max_globals: " << max_globals << std::endl;
  out << indent << "hash_tables.total_globals: " << global_vars_ << std::endl;
  out << indent << "hash_tables.max_libs: " << max_libs << std::endl;
  out << indent << "hash_tables.total_libs: " << total_libs << std::endl;
  out << indent << "hash_tables.max_modulites: " << max_modulites << std::endl;
  out << indent << "hash_tables.total_modulites: " << total_modulites << std::endl;
  out << indent << "hash_tables.max_composer_jsons: " << max_composer_jsons << std::endl;
  out << indent << "hash_tables.total_composer_jsons: " << total_composer_jsons << std::endl;
  out << block_sep;
  out << std::fixed;
  for (const auto &prof : profiler_stats) {
    std::string name = prof.first;
    std::replace_if(name.begin(), name.end(), [](char c) { return !std::isalnum(c); }, '_');
    out << "pipes." << name << ".working_time: " << std::chrono::duration<double>(prof.second.get_working_time()).count() << std::endl;
    out << "pipes." << name << ".duration: " << std::chrono::duration<double>(prof.second.get_duration()).count() << std::endl;
    out << "pipes." << name << ".memory_usage: " << prof.second.get_memory_usage() << std::endl;
    out << "pipes." << name << ".memory_allocated: " << prof.second.get_memory_total_allocated() << std::endl;
    out << "pipes." << name << ".calls: " << prof.second.get_calls() << std::endl;
  }
  out.unsetf(std::ios_base::floatfield);
}
