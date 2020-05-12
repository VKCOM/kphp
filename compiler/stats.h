#pragma once

#include <atomic>
#include <ostream>

#include "compiler/data/var-data.h"

class Stats {
public:
  void on_var_inserting(VarData::Type type);
  void on_function_processed(FunctionPtr function);

  void write_to(std::ostream &out);

  std::atomic<std::uint64_t> total_classes{0u};
  std::atomic<std::uint64_t> total_lambdas{0u};
  std::atomic<std::uint64_t> cnt_mixed_params{0u};
  std::atomic<std::uint64_t> cnt_mixed_vars{0u};
  std::atomic<std::uint64_t> cnt_const_mixed_params{0u};
  std::atomic<std::uint64_t> cnt_make_clone{0u};

private:
  std::atomic<std::uint64_t> local_vars_{0u};
  std::atomic<std::uint64_t> local_inplace_vars_{0u};
  std::atomic<std::uint64_t> global_vars_{0u};
  std::atomic<std::uint64_t> global_const_vars_{0u};
  std::atomic<std::uint64_t> param_vars_{0u};
  std::atomic<std::uint64_t> static_vars_{0u};
  std::atomic<std::uint64_t> instance_vars_{0u};
  std::atomic<std::uint64_t> total_functions_{0u};
  std::atomic<std::uint64_t> total_throwing_functions_{0u};
  std::atomic<std::uint64_t> total_resumable_functions_{0u};
};


