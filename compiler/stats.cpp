#include "compiler/stats.h"

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
void Stats::write_to(std::ostream &out) {
  out << std::endl;
  out << "Compile stats:" << std::endl;
  out << "  total classes: " << total_classes << std::endl;
  out << "  total lambdas: " << total_lambdas << std::endl;
  out << "  local vars: " << local_vars_ << std::endl;
  out << "  local inplace vars: " << local_inplace_vars_ << std::endl;
  out << "  global vars: " << global_vars_ << std::endl;
  out << "  global const vars: " << global_const_vars_ << std::endl;
  out << "  param vars: " << param_vars_ << std::endl;
  out << "  static vars: " << static_vars_ << std::endl;
  out << "  instance vars: " << instance_vars_ << std::endl;
  out << "  count mixed vars: " << cnt_mixed_vars << std::endl;
  out << "  count mixed params: " << cnt_mixed_params << std::endl;
  out << std::endl;
}
