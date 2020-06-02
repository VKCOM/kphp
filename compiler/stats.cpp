#include "compiler/stats.h"

#include "drinkless/dl-utils-lite.h"

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
  if (function->can_throw) {
    ++total_throwing_functions_;
  }
  if (function->is_resumable) {
    ++total_resumable_functions_;
  }
  if (function->is_inline) {
    ++total_inline_functions_;
  }
}

void Stats::update_memory_stats() {
  mem_info_t mem_info;
  get_mem_stats(getpid(), &mem_info);

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
  out << block_sep;
  out << indent << "memory.rss_kb: " << memory_rss_ << std::endl;
  out << indent << "memory.rss_peak_kb: " << memory_rss_peak_ << std::endl;
  out << block_sep;
  out << indent << "compilation.total_time: " << total_time << std::endl;
  out << indent << "compilation.object_out_size: " << object_out_size << std::endl;
  out << block_sep;
  for (const auto &prof : profiler_stats) {
    if (prof.get_count() > 0 && prof.get_working_time().count() > 0) {
      std::string name = prof.name;
      std::replace_if(name.begin(), name.end(), [](char c) { return !std::isalnum(c); }, '_');
      out << "pipes." << name << ".total_time: " << std::chrono::duration<double>(prof.get_working_time()).count() << std::endl;
      out << "pipes." << name << ".passed_functions: " << prof.get_count() << std::endl;
    }
  }
}
