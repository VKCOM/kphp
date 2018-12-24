#pragma once

#include <cstddef>
#include <cstdio>

#include "common/cycleclock.h"

#include "compiler/threading/tls.h"

#define TACT_SPEED (1e-6 / 2266.0)

class ProfilerRaw {
private:
  long long count;
  long long ticks;
  size_t memory;
  int flag;
public:
  void alloc_memory(size_t size) {
    count++;
    memory += size;
  }

  size_t get_memory() {
    return memory;
  }

  void start() {
    if (flag == 0) {
      ticks -= cycleclock_now();
      count++;
    }
    flag++;
  }

  void finish() {
    flag--;
    if (flag == 0) {
      ticks += cycleclock_now();
    }
  }

  long long get_ticks() {
    return ticks;
  }

  long long get_count() {
    return count;
  }

  double get_time() {
    return get_ticks() * TACT_SPEED;
  }
};


#define PROF_E_(x) prof_ ## x
#define PROF_E(x) PROF_E_(x)
#define FOREACH_PROF(F)\
  F (next_name)\
  F (next_const_string_name)\
  F (create_function)\
  F (load_files)\
  F (lexer)\
  F (gentree)\
  F (gentree_postprocess)\
  F (split_switch)\
  F (create_switch_vars)\
  F (collect_required)\
  F (sort_and_inherit_classes)\
  F (calc_locations)\
  F (resolve_self_static_parent)\
  F (register_defines)\
  F (calc_real_defines_values)\
  F (erase_defines_declarations)\
  F (inline_defines_usages)\
  F (check_returns)\
  F (preprocess_eq3)\
  F (preprocess_function_c)\
  F (preprocess_break)\
  F (register_variables)\
  F (check_instance_props)\
  F (calc_const_type)\
  F (collect_const_vars)\
  F (calc_actual_calls_edges)\
  F (calc_actual_calls)\
  F (calc_throws_and_body_value)\
  F (check_function_calls)\
  F (calc_rl)\
  F (CFG)\
  F (type_infence)\
  F (tinf_infer)\
  F (tinf_infer_gen_dep)\
  F (tinf_infer_infer)\
  F (tinf_find_isset)\
  F (tinf_check)\
  F (CFG_End)\
  F (optimization)\
  F (calc_val_ref)\
  F (calc_func_dep)\
  F (calc_bad_vars)\
  F (check_ub)\
  F (analizer)\
  F (extract_resumable_calls)\
  F (extract_async)\
  F (check_access_modifiers)\
  F (final_check)\
  F (code_gen)\
  F (writer)\
  F (end_write)\
  F (make)\
  F (sync_with_dir)\
  F (vertex_inner)\
  F (vertex_inner_data)\
  F (total)

#define DECLARE_PROF_E(x) PROF_E(x),
enum ProfilerId {
  FOREACH_PROF (DECLARE_PROF_E)
  ProfilerId_size
};

class Profiler {
public:
  ProfilerRaw raw[ProfilerId_size];
  char dummy[4096];
};

extern TLS<Profiler> profiler;

inline ProfilerRaw &get_profiler(ProfilerId id) {
  return profiler->raw[id];
}

#define PROF(x) get_profiler (PROF_E (x))

void profiler_print_all();

template<ProfilerId Id>
class AutoProfiler {
private:
  ProfilerRaw &prof;
public:
  AutoProfiler() :
    prof(get_profiler(Id)) {
    prof.start();
  }

  ~AutoProfiler() {
    prof.finish();
  }
};

#define AUTO_PROF(x) AutoProfiler <PROF_E (x)> x ## _auto_prof
