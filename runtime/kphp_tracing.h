// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

namespace kphp_tracing {

class tracing_binary_buffer {
  struct one_chunk {
    int *buf;
    size_t size_bytes;
    one_chunk *prev_chunk;
  };

  one_chunk *cur_chunk{nullptr};
  int *pos{nullptr};  // points inside cur_chunk->buf

public:

  void init_and_alloc();
  void alloc_next_chunk_if_not_enough(int reserve_bytes);
  void finish_cur_chunk_start_next();
  void finish_cur_buffer();
  void clear();

  int get_cur_chunk_size() const { return (pos - cur_chunk->buf) * 4; }

  void write_int32(int64_t v) { *pos++ = static_cast<int>(v); }
  void write_int64(int64_t v);
  void write_uint32(int64_t v) { *pos++ = static_cast<unsigned int>(v); }
  void write_float32(double v) { float f32 = static_cast<float>(v); *pos++ = *reinterpret_cast<int *>(&f32); }
  void write_float64(double v);

  int64_t register_string_in_table(const string &v);
  void write_string_ref_table(int64_t idx_in_table) { *pos++ = static_cast<int>(idx_in_table) << 8; }
  void write_string_inlined(const string &v);

  void prepend_dict_buffer(const tracing_binary_buffer &dict_buffer);
  void output_to_json_log(const string &json_line);
};

extern tracing_binary_buffer trace_binlog;
extern bool vslice_runtime_enabled;

void init_tracing_lib();
void free_tracing_lib();

} // namespace

void f$kphp_tracing_init_binlog();

void f$kphp_tracing_write_event_type(int64_t event_type, int64_t custom24bits);
inline void f$kphp_tracing_write_int32(int64_t v) { kphp_tracing::trace_binlog.write_int32(v); }
inline void f$kphp_tracing_write_int64(int64_t v) { kphp_tracing::trace_binlog.write_int64(v); }
inline void f$kphp_tracing_write_uint32(int64_t v) { kphp_tracing::trace_binlog.write_uint32(v); }
inline void f$kphp_tracing_write_float32(double v) { kphp_tracing::trace_binlog.write_float32(v); }
inline void f$kphp_tracing_write_float64(double v) { kphp_tracing::trace_binlog.write_float64(v); }

inline int64_t f$kphp_tracing_register_string_if_const(const string &v) {
  if (!v.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return 0;
  }
  return kphp_tracing::trace_binlog.register_string_in_table(v);
}

inline int64_t f$kphp_tracing_register_string_force(const string &v) {
  return kphp_tracing::trace_binlog.register_string_in_table(v);
}

inline void f$kphp_tracing_write_string(const string &v, int64_t idx_in_table) {
  if (idx_in_table > 0) {
    kphp_tracing::trace_binlog.write_string_ref_table(idx_in_table);
  } else {
    kphp_tracing::trace_binlog.write_string_inlined(v);
  }
}


void f$kphp_tracing_enable_vslice_collecting();

class C$KphpTracingVSliceAtRuntime {
public:
  int refcnt{0};
  int vsliceID;
  size_t start_memory_used;
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

void f$kphp_tracing_write_trace_to_json_log(const string &jsonTraceLine);
void f$kphp_tracing_prepend_dict_to_binlog(int64_t dictID, const array<string> &dictKV);
