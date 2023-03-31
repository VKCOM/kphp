// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing.h"

#include <chrono>
#include <unordered_map>

#include "runtime/critical_section.h"
#include "runtime/runtime_injection.h"
#include "server/json-logger.h"


namespace kphp_tracing {

constexpr int INITIAL_SIZE_BYTES = 262144;
constexpr int EXPAND_SIZE_BYTES = 65536;

constexpr int64_t etStringsTableRegister = 1;
constexpr int64_t etDictClearNumbers = 5;
constexpr int64_t etDictSetNumberMeaning = 6;

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
    deallocate();
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

void tracing_binary_buffer::deallocate() {
  dl::deallocate(buf, capacity);
  clear();
}

const unsigned char *tracing_binary_buffer::get_raw_bytes() const {
  return reinterpret_cast<const unsigned char *>(buf);
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

void f$kphp_tracing_write_trace_to_json_log(const string &jsonTraceLine) {
  vk::string_view json_no_braces = vk::string_view{jsonTraceLine.c_str(), jsonTraceLine.size()};
  json_no_braces.remove_prefix(1);
  json_no_braces.remove_suffix(1);

  auto &json_logger = vk::singleton<JsonLogger>::get();
  fprintf(stderr, "out buf %d\n", kphp_tracing::trace_binlog.size_bytes());
  dl::enter_critical_section();
  json_logger.write_trace_line(json_no_braces, kphp_tracing::trace_binlog.get_raw_bytes(), kphp_tracing::trace_binlog.size_bytes());
  dl::leave_critical_section();
}

// todo try to delete
void f$kphp_tracing_write_dict_to_json_log(int64_t dictID, int64_t protocolVersion, int64_t pid, const array<string> &dictKV) {
  kphp_tracing::tracing_binary_buffer dict_buffer;
  dict_buffer.init_and_alloc();
  dict_buffer.write_int32((dictID << 8) + kphp_tracing::etDictClearNumbers);
  for (const auto &it : dictKV) {
    dict_buffer.write_int32((dictID << 8) + kphp_tracing::etDictSetNumberMeaning);
    dict_buffer.write_uint32(it.get_int_key());
    dict_buffer.write_string_inlined(it.get_value());
  }

  string json = string(R"("trace_id":"00000000000000000000000000000000","pid":)");
  json.append(pid);
  json.append(R"(,"protocol_v":)");
  json.append(protocolVersion);
  vk::string_view json_no_braces = vk::string_view{json.c_str(), json.size()};

  auto &json_logger = vk::singleton<JsonLogger>::get();
  dl::enter_critical_section();
  json_logger.write_trace_line(json_no_braces, dict_buffer.get_raw_bytes(), dict_buffer.size_bytes());
  dl::leave_critical_section();

  dict_buffer.deallocate();
}

