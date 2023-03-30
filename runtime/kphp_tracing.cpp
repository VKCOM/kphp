// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"

#include <chrono>
#include <unordered_map>

#include "runtime/critical_section.h"
#include "runtime/runtime_injection.h"


extern const char lhex_digits[17];

namespace kphp_tracing {

constexpr int INITIAL_SIZE_BYTES = 262144;
constexpr int EXPAND_SIZE_BYTES = 65536;
constexpr int64_t etStringsTableRegister = 37;

tracing_binary_buffer trace_binlog;
bool vslice_runtime_enabled;
std::unordered_map<int64_t, int> sent_strings_table;


void init_tracing_lib() {
  // there is nothing to init:
  // if PHP code requires tracing, it calls kphp_tracing_init_binlog() and registered
}

void free_tracing_lib() {
  trace_binlog.clear();
  vslice_runtime_enabled = false;
}


void tracing_binary_buffer::init_and_alloc() {
  if (buf != nullptr) {
    dl::deallocate(buf, capacity);
  }

  buf = reinterpret_cast<int *>(dl::allocate(INITIAL_SIZE_BYTES));
  pos = buf;
  capacity = INITIAL_SIZE_BYTES;
}

void tracing_binary_buffer::alloc_if_not_enough(int reserve_bytes) {
  if (unlikely(buf == nullptr)) {
    php_critical_error("tracing not initialized, but used");
  }
  int cur_size = size_bytes();
  if (cur_size + reserve_bytes > capacity) {
    int offset = pos - buf;
    buf = reinterpret_cast<int *>(dl::reallocate(buf, capacity + EXPAND_SIZE_BYTES, capacity));
    pos = buf + offset;
    capacity += EXPAND_SIZE_BYTES;
  }
}

void tracing_binary_buffer::clear() {
  // no need of dl::deallocate(), because clear() is called at the end of the script,
  // we just need to clear pointers, so that dl::allocate() is called again for a new script
  buf = nullptr;
  pos = nullptr;
  capacity = 0;
}

void tracing_binary_buffer::write_int64(int64_t v) {
  int64_t *pos64 = reinterpret_cast<int64_t *>(pos);
  *pos64 = v;
  pos += 2;
}

void tracing_binary_buffer::write_float64(double v) {
  int64_t *pos64 = reinterpret_cast<int64_t *>(pos);
  *pos64 = *reinterpret_cast<int64_t *>(&v);
  pos += 2;
}

int64_t tracing_binary_buffer::register_string_in_table(const string &v) {
  if (unlikely(v.size() > 255)) {
    php_warning("too large string for a tracing buffer");
    return 0;
  }

  int64_t hash64 = v.hash();
  const auto it = sent_strings_table.find(hash64);
  if (it != sent_strings_table.end()) {
    return it->second;
  }

  unsigned int idx_in_table = sent_strings_table.size() + 1;
  dl::enter_critical_section();
  sent_strings_table[hash64] = idx_in_table;
  dl::leave_critical_section();
  f$kphp_tracing_write_event_type(etStringsTableRegister, idx_in_table);

  write_string_inlined(v);
  return idx_in_table;
}

void tracing_binary_buffer::write_string_inlined(const string &v) {
  if (unlikely(v.size() > 255)) {
    php_warning("too large string for a tracing buffer");
    write_int32(0);
    return;
  }

  write_int32(v.size());
  alloc_if_not_enough(512);

  char *pos8 = reinterpret_cast<char *>(pos);
  memcpy(pos8, v.c_str(), v.size());
  pos += (v.size() + 3) / 4;  // a string is rounded up to 4 bytes (len 7 -> consumes 8)
}

} // namespace

void f$kphp_tracing_init_binlog() {
  kphp_tracing::trace_binlog.init_and_alloc();
}

string f$kphp_tracing_get_binlog_as_hex_string() {
  string out;
  if (kphp_tracing::trace_binlog.buf != nullptr) {
    int buf_size = kphp_tracing::trace_binlog.size_bytes();
    const unsigned char *p = reinterpret_cast<const unsigned char *>(kphp_tracing::trace_binlog.buf);
    const unsigned char *end = p + buf_size;
    out.reserve_at_least(buf_size * 2);
    for (; p != end; ++p) {
      out.push_back(lhex_digits[(*p & 0xF0) >> 4]);
      out.push_back(lhex_digits[(*p & 0x0F)]);
    }
  }
  return out;
}

void f$kphp_tracing_write_event_type(int64_t event_type, int64_t custom24bits) {
  if (unlikely(custom24bits < 0 || custom24bits >= 1 << 24)) {
    php_warning("custom24bits overflow next to event_type");
  }
  kphp_tracing::trace_binlog.alloc_if_not_enough(128);
  kphp_tracing::trace_binlog.write_uint32((custom24bits << 8) + event_type);
}

void f$kphp_tracing_enable_vslice_collecting() {
  kphp_tracing::vslice_runtime_enabled = true;
}

void C$KphpTracingVSliceAtRuntime::add_ref() {
  if (refcnt++ == 0) {
    double now_timestamp = std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count();
    start_memory_used = dl::get_script_memory_stats().memory_used;
    runtime_injection::invoke_callback(runtime_injection::on_tracing_vslice_start, vsliceID, now_timestamp);
  }
}

void C$KphpTracingVSliceAtRuntime::release() {
  if (--refcnt == 0) {
    double now_timestamp = std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count();
    size_t memory_used = dl::get_script_memory_stats().memory_used - start_memory_used;
    runtime_injection::invoke_callback(runtime_injection::on_tracing_vslice_finish, vsliceID, now_timestamp, memory_used);
  }
}
