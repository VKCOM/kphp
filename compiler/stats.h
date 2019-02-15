#pragma once
#include "compiler/data/var-data.h"

#include <atomic>
#include <ostream>

class Stats {
public:
  void on_var_inserting(VarData::Type type);

  void write_to(std::ostream &out);

  std::atomic<std::uint64_t> total_classes{0u};
  std::atomic<std::uint64_t> total_lambdas{0u};

private:
  std::atomic<std::uint64_t> local_vars_{0u};
  std::atomic<std::uint64_t> local_inplace_vars_{0u};
  std::atomic<std::uint64_t> global_vars_{0u};
  std::atomic<std::uint64_t> global_const_vars_{0u};
  std::atomic<std::uint64_t> param_vars_{0u};
  std::atomic<std::uint64_t> static_vars_{0u};
  std::atomic<std::uint64_t> instance_vars_{0u};
};


