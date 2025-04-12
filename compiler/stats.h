// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <ostream>

#include "compiler/data/var-data.h"
#include "compiler/threading/profiler.h"

class Stats {
public:
  void on_var_inserting(VarData::Type type);
  void on_function_processed(FunctionPtr function);

  void update_memory_stats();

  void write_to(std::ostream &out, bool with_indent = true) const;

  std::atomic<std::uint64_t> total_classes{0u};
  std::atomic<std::uint64_t> total_lambdas{0u};
  std::atomic<std::uint64_t> cnt_mixed_params{0u};
  std::atomic<std::uint64_t> cnt_mixed_vars{0u};
  std::atomic<std::uint64_t> cnt_const_mixed_params{0u};
  std::atomic<std::uint64_t> cnt_make_clone{0u};

  std::atomic<std::uint64_t> object_out_size{0u};
  std::atomic<double> transpilation_time{0.0};
  std::atomic<double> total_time{0.0};

  // Counters for hash tables' sizes in CompilerCore
  std::atomic<std::uint64_t> ht_total_files{0u};
  std::atomic<std::uint64_t> ht_total_dirs{0u};
  std::atomic<std::uint64_t> ht_total_functions{0u};
  std::atomic<std::uint64_t> ht_total_classes{0u};
  std::atomic<std::uint64_t> ht_total_defines{0u};
  std::atomic<std::uint64_t> ht_total_constants{0u};
  std::atomic<std::uint64_t> ht_total_globals{0u};
  std::atomic<std::uint64_t> ht_total_libs{0u};
  std::atomic<std::uint64_t> ht_total_modulites{0u};
  std::atomic<std::uint64_t> ht_total_composer_jsons{0u};

  // Max sizes of hash tables in CompilerCore
  std::atomic<std::uint64_t> ht_max_files{0u};
  std::atomic<std::uint64_t> ht_max_dirs{0u};
  std::atomic<std::uint64_t> ht_max_functions{0u};
  std::atomic<std::uint64_t> ht_max_classes{0u};
  std::atomic<std::uint64_t> ht_max_defines{0u};
  std::atomic<std::uint64_t> ht_max_constants{0u};
  std::atomic<std::uint64_t> ht_max_globals{0u};
  std::atomic<std::uint64_t> ht_max_libs{0u};
  std::atomic<std::uint64_t> ht_max_modulites{0u};
  std::atomic<std::uint64_t> ht_max_composer_jsons{0u};

  std::unordered_map<std::string, ProfilerRaw> profiler_stats;

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
  std::atomic<std::uint64_t> total_interruptible_functions_{0u};
  std::atomic<std::uint64_t> total_inline_functions_{0u};

  std::atomic<std::uint64_t> memory_rss_{0};
  std::atomic<std::uint64_t> memory_rss_peak_{0};
};


