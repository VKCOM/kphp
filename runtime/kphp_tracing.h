// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

namespace kphp_tracing {

struct tracing_binary_buffer {
  int capacity{0};
  int *buf{nullptr};
  int *pos{nullptr};

  void init_and_alloc();
  void alloc_if_not_enough(int reserve_bytes);
  void clear();

  int size_bytes() const { return (pos - buf) * 4; }

  void write_int32(int64_t v) { *pos++ = static_cast<int>(v); }
  void write_int64(int64_t v);
  void write_uint32(int64_t v) { *pos++ = static_cast<unsigned int>(v); }
  void write_float32(double v) { float f32 = static_cast<float>(v); *pos++ = *reinterpret_cast<int *>(&f32); }
  void write_float64(double v);
  void write_string(const string &v);
};

extern tracing_binary_buffer trace_binlog;
extern bool vslice_runtime_enabled;

void init_tracing_lib();
void free_tracing_lib();

}

void f$kphp_tracing_init_binlog();
string f$kphp_tracing_get_binlog_as_hex_string();

void f$kphp_tracing_write_event_type(int64_t event_type, int64_t custom24bits);
inline void f$kphp_tracing_write_int32(int64_t v) { kphp_tracing::trace_binlog.write_int32(v); }
inline void f$kphp_tracing_write_int64(int64_t v) { kphp_tracing::trace_binlog.write_int64(v); }
inline void f$kphp_tracing_write_uint32(int64_t v) { kphp_tracing::trace_binlog.write_uint32(v); }
inline void f$kphp_tracing_write_float32(double v) { kphp_tracing::trace_binlog.write_float32(v); }
inline void f$kphp_tracing_write_float64(double v) { kphp_tracing::trace_binlog.write_float64(v); }
inline void f$kphp_tracing_write_string(const string &v) { kphp_tracing::trace_binlog.write_string(v); }


void f$kphp_tracing_enable_vslice_collecting();

class C$KphpTracingVSliceAtRuntime {
public:
  int refcnt{0};
  int vsliceID;
  double start_timestamp;
  int allocations_count;
  int allocated_bytes;
  explicit C$KphpTracingVSliceAtRuntime(int vsliceID): vsliceID(vsliceID) {}

  void add_ref();
  void release();
};


inline __attribute__((always_inline)) class_instance<C$KphpTracingVSliceAtRuntime> f$kphp_tracing_vslice(int vsliceID) {
  class_instance<C$KphpTracingVSliceAtRuntime> g;
  if (unlikely(kphp_tracing::vslice_runtime_enabled)) {
    g.alloc(vsliceID);
  }
  return g;
}
