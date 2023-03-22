// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"

extern const char lhex_digits[17];

namespace kphp_tracing {

constexpr int INITIAL_SIZE_BYTES = 262144;
constexpr int EXPAND_SIZE_BYTES = 65536;

on_fork_switch_callback_t on_fork_start;
on_fork_state_change_callback_t on_fork_finish;
on_fork_switch_callback_t on_fork_switch;
on_rpc_request_start_callback_t on_rpc_request_start;
on_rpc_request_finish_callback_t on_rpc_request_finish;
on_job_request_start_callback_t on_job_request_start;
on_job_request_finish_callback_t on_job_request_finish;
on_net_to_script_switch_callback_t on_net_to_script_ctx_switch;

tracing_binary_buffer trace_binlog;


void init_tracing_lib() {
  // there is nothing to init:
  // if PHP code requires tracing, it calls kphp_tracing_init_binlog() and registered
}

void tree_tracing_lib() {
  on_fork_start = {};
  on_fork_finish = {};
  on_fork_switch = {};
  on_rpc_request_start = {};
  on_rpc_request_finish = {};
  on_job_request_start = {};
  on_job_request_finish = {};
  on_net_to_script_ctx_switch = {};

  trace_binlog.clear();
}


void tracing_binary_buffer::init_and_alloc() {
  buf = reinterpret_cast<int *>(dl::allocate(INITIAL_SIZE_BYTES));
  pos = buf;
  capacity = INITIAL_SIZE_BYTES;
  // todo delete after comparing with php
  memset(reinterpret_cast<char *>(buf), 0, capacity);
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
    // todo delete after comparing with php
    memset(reinterpret_cast<char *>(buf) + capacity - EXPAND_SIZE_BYTES, 0, EXPAND_SIZE_BYTES);
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

void tracing_binary_buffer::write_string(const string &v) {
  if (unlikely(v.size() > 128)) {
    php_warning("too large string for a tracing buffer");
    write_int32(0);
    return;
  }

  alloc_if_not_enough(256);
  write_int32(v.size());

  char *pos8 = reinterpret_cast<char *>(pos);
  memcpy(pos8, v.c_str(), v.size());
  pos += (v.size() + 3) / 4;  // a string is rounded up to 4 bytes (len 7 -> consumes 8)
}

void tracing_binary_buffer::write_bool(bool v) {
  *pos++ = static_cast<int>(v);
}

}

void f$register_kphp_on_fork_callbacks(const kphp_tracing::on_fork_switch_callback_t &on_fork_start,
                                       const kphp_tracing::on_fork_state_change_callback_t &on_fork_finish,
                                       const kphp_tracing::on_fork_switch_callback_t &on_fork_switch) {
  kphp_tracing::on_fork_start = on_fork_start;
  kphp_tracing::on_fork_finish = on_fork_finish;
  kphp_tracing::on_fork_switch = on_fork_switch;
}

void f$register_kphp_on_rpc_query_callbacks(const kphp_tracing::on_rpc_request_start_callback_t &on_start,
                                            const kphp_tracing::on_rpc_request_finish_callback_t &on_finish) {
  kphp_tracing::on_rpc_request_start = on_start;
  kphp_tracing::on_rpc_request_finish = on_finish;
}

void f$register_kphp_on_job_worker_callbacks(const kphp_tracing::on_job_request_start_callback_t &on_start,
                                             const kphp_tracing::on_job_request_finish_callback_t &on_finish) {
  kphp_tracing::on_job_request_start = on_start;
  kphp_tracing::on_job_request_finish = on_finish;
}

void f$register_kphp_on_swapcontext_callbacks(const kphp_tracing::on_net_to_script_switch_callback_t &on_net_to_script_switch) {
  kphp_tracing::on_net_to_script_ctx_switch = on_net_to_script_switch;
}

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
  if (unlikely(custom24bits < 0 || custom24bits >= 1<<24)) {
    php_warning("custom24bits overflow next to event_type");
  }
  kphp_tracing::trace_binlog.alloc_if_not_enough(128);
  kphp_tracing::trace_binlog.write_uint32((custom24bits << 8) + event_type);
}
