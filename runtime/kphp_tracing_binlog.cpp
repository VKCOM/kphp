// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_tracing_binlog.h"

#include <unordered_map>
#include <forward_list>

#include "runtime/critical_section.h"
#include "server/json-logger.h"


extern const char lhex_digits[17];

namespace kphp_tracing {

// when updating binlog protocol (and decoder), don't forget to bump this version
// (to make decoder correctly read both old and new binlog data)
constexpr int BINLOG_VERSION = 102;

// kphp binlog buffer is represented as a list of chunks
// when a chunk comes to an end, another one is allocated from script memory
constexpr int BUF_CHUNK_SIZE = 65536;

// for optimization, binlog protocol allows registering strings to reuse them later:
// attribute names, span titles and others are often const strings in php code,
// so when such a string occurs the first time, it's registered (acquires idxInTable), and later,
// when that string is needed to be sent, only its index is sent; see `write_string_inlined()`;
// `worker_process_strings_table` contains strings table throughout all worker lifetime;
// note, that we don't store strings themselves, only int64 hashes: it's enough
// (storing strings would require copying them somewhere outside script memory)
static std::unordered_map<int64_t, int> worker_process_strings_table;

// the purpose of the following variable is quite hard to be understood;
// remember, that every request (every script execution) is traced in memory,
// and in the end (rarely) its trace is flushed to a file;
// it's a common situation that during script execution, new binlog strings
// are registered in `worker_process_strings_table`;
// but if the trace is NOT flushed (just dropped), decoder will not receive that strings,
// so later on it won't be able to correctly decode some strings in future traces;
// to overcome this problem, when the trace is NOT flushed, and if the request did register new strings,
// that new strings are flushed separately;
// there are 2 ways to flush them:
// 1) after every request that had registered new strings, push them into a file
// 2) keep them in memory and push all at once just before any trace that will be flushed later
// the case (2) is what we are focusing on; we are keeping new strings in memory until any other trace flushed -
// but not as "string" (that would require copying from script memory), but as a separate full-fledged binlog
// with events type=etStringsTableRegister and inlined strings;
// in kphp_tracing.cpp, there is a variable:
// `static tracing_binary_buffer flush_strings_binlog;`
// if a trace is not flushed, `flush_strings_binlog.append_current_php_script_strings()` is called;
// when inserting to that binlog, we take memory from heap using dl::heap_allocate()
// so that it outlives current script execution;
// before any trace flush, if that binlog is not empty, it's flushed in advance and cleared
static std::vector<string> current_php_script_registered_strings;


void tracing_binary_buffer::set_use_heap_memory() {
  use_heap_memory = true;
}

void tracing_binary_buffer::init_and_alloc() {
  finish_cur_chunk_start_next();
}

void tracing_binary_buffer::alloc_next_chunk_if_not_enough(int reserve_bytes) {
  if (unlikely(last_chunk == nullptr)) {
    php_critical_error("tracing not initialized, but used");
  }
  int cur_size = get_cur_chunk_size();
  if (cur_size + reserve_bytes > BUF_CHUNK_SIZE) {
    finish_cur_chunk_start_next();
  }
}

void tracing_binary_buffer::finish_cur_chunk_start_next() {
  if (last_chunk != nullptr) {
    last_chunk->size_bytes = get_cur_chunk_size();
  }

  void *mem = use_heap_memory ? dl::heap_allocate(BUF_CHUNK_SIZE) : dl::allocate(BUF_CHUNK_SIZE);
  one_chunk *next_chunk = new (mem) one_chunk{
    reinterpret_cast<int *>(reinterpret_cast<char *>(mem) + sizeof(one_chunk)),
    0,
    last_chunk
  };

  last_chunk = next_chunk;
  pos = last_chunk->buf;
}

void tracing_binary_buffer::clear(bool real_deallocate) {
  // for script memory, no need of dl::deallocate(), because clear() is called at the end of the script,
  // we just need to reset pointers, so that dl::allocate() is called again for a new script;
  // but for heap memory, we need to really deallocate all chunks

  if (real_deallocate) {
    for (one_chunk *chunk = last_chunk; chunk;) {
      one_chunk *prev_chunk = chunk->prev_chunk;
      dl::heap_deallocate(chunk, BUF_CHUNK_SIZE);
      chunk = prev_chunk;
    }
  }

  last_chunk = nullptr;
  pos = nullptr;
}

void tracing_binary_buffer::write_event_type(EventTypeEnum event_type, int custom24bits) {
  if (unlikely(custom24bits < 0 || custom24bits >= 1 << 24)) {
    php_warning("custom24bits %d overflow next to event_type %d", custom24bits, static_cast<int>(event_type));
  }

  // 128 bytes are enough for all possible events in binlog, they are tiny in fact, 24 bytes max
  // write_event_type() is called for every event (see BinlogWriter methods),
  // and checks inside write_int() and other primitive functions aren't presented intentionally
  // if an event will contain an inlined string (not an indexed one), write_string_inlined() will also check size
  alloc_next_chunk_if_not_enough(128);
  write_uint32(static_cast<unsigned int>(custom24bits << 8) + static_cast<unsigned int>(event_type));
}

int tracing_binary_buffer::calc_total_size() const {
  int binlog_size = get_cur_chunk_size();
  for (const one_chunk *chunk = last_chunk->prev_chunk; chunk; chunk = chunk->prev_chunk) {
    binlog_size += chunk->size_bytes;
  }
  return binlog_size;
}

void tracing_binary_buffer::write_int64(int64_t v) {
  int64_t *pos64 = reinterpret_cast<int64_t *>(pos);
  *pos64 = v;
  pos += 2;
}

void tracing_binary_buffer::write_uint64(uint64_t v) {
  uint64_t *pos64 = reinterpret_cast<uint64_t *>(pos);
  *pos64 = v;
  pos += 2;
}

void tracing_binary_buffer::write_float64(double v) {
  int64_t *pos64 = reinterpret_cast<int64_t *>(pos);
  *pos64 = *reinterpret_cast<int64_t *>(&v);
  pos += 2;
}

int tracing_binary_buffer::register_string_in_table(const string &v) {
  if (unlikely(v.size() > 255)) {
    php_warning("too large string for register_string_in_table, len %d", v.size());
    return 0;
  }

  int64_t hash64 = v.hash();
  const auto it = worker_process_strings_table.find(hash64);
  if (it != worker_process_strings_table.end()) {
    return it->second;
  }

  int idx_in_table = worker_process_strings_table.size() + 1;
  dl::enter_critical_section();
  worker_process_strings_table[hash64] = idx_in_table;
  current_php_script_registered_strings.push_back(v);
  dl::leave_critical_section();
  write_event_type(EventTypeEnum::etStringsTableRegister, idx_in_table);

  write_string_inlined(v);
  return idx_in_table;
}

void tracing_binary_buffer::write_string_inlined(const string &v) {
  if (unlikely(v.size() > 127)) {
    string cut = v.substr(0, 127);
    write_string_inlined(cut);
    return;
  }

  write_int32(static_cast<int>(v.size()));
  alloc_next_chunk_if_not_enough(256);

  char *pos8 = reinterpret_cast<char *>(pos);
  memcpy(pos8, v.c_str(), v.size());
  pos += (v.size() + 3) / 4;  // a string is rounded up to 4 bytes (len 7 -> consumes 8)
}

void tracing_binary_buffer::append_enum_values(int enumID, const string &enumName, const array<string> &enumKV) {
  if (last_chunk == nullptr) {
    init_and_alloc();
  }

  write_event_type(EventTypeEnum::etEnumCreateBlank, enumID);
  write_string_inlined(enumName);
  for (const auto &it : enumKV) {
    int64_t key = enumKV.is_vector() ? it.get_key().to_int() : it.get_int_key();
    write_event_type(EventTypeEnum::etEnumSetMeaning, enumID);
    write_uint32(static_cast<unsigned int>(key));
    write_string_inlined(it.get_value());
  }
}

// see comment next to `current_php_script_registered_strings` declaration
void tracing_binary_buffer::append_current_php_script_strings() {
  if (current_php_script_registered_strings.empty()) {
    return;
  }
  if (last_chunk == nullptr) {
    init_and_alloc();
  }

  for (const string &v : current_php_script_registered_strings) {
    int idx_in_table = worker_process_strings_table[v.hash()];
    write_event_type(EventTypeEnum::etStringsTableRegister, idx_in_table);
    write_string_inlined(v);
  }
}

// final json line will look like this:
// {...(json_without_binlog),"pid":%d,"binlog_v":%d,"binlog":"...hex..."}\n
void tracing_binary_buffer::output_to_json_log(const char *json_without_binlog) {
  if (last_chunk == nullptr) {
    return;
  }
  dl::CriticalSectionGuard lock;
  last_chunk->size_bytes = get_cur_chunk_size();

  size_t binlog_size = 0;
  std::forward_list<const one_chunk *> chunks_ordered;
  for (const one_chunk *chunk = last_chunk; chunk; chunk = chunk->prev_chunk) {
    chunks_ordered.emplace_front(chunk);
    binlog_size += chunk->size_bytes;
  }

  size_t custom_strlen = strlen(json_without_binlog);
  size_t buffer_capacity = custom_strlen + 128 + binlog_size * 2;
  char *buffer = new char[buffer_capacity];
  int buffer_i = 0;

  memcpy(buffer, json_without_binlog, custom_strlen - 1);
  buffer_i += custom_strlen - 1;
  buffer_i += snprintf(buffer + buffer_i, buffer_capacity, R"(,"pid":%d,"binlog_v":%d,"binlog":")", static_cast<int>(getpid()), BINLOG_VERSION);

  for (const auto &it : chunks_ordered) {
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(it->buf), *end = p + it->size_bytes; p != end; ++p) {
      buffer[buffer_i++] = lhex_digits[(*p & 0xF0) >> 4];
      buffer[buffer_i++] = lhex_digits[(*p & 0x0F)];
    }
  }

  buffer[buffer_i++] = '"';
  buffer[buffer_i++] = '}';
  buffer[buffer_i++] = '\n';
  assert(buffer_i < buffer_capacity);

  JsonLogger &json_logger = vk::singleton<JsonLogger>::get();
  json_logger.write_trace_line(buffer, buffer_i);
  delete[] buffer;
}

void free_tracing_binlog_lib() {
  current_php_script_registered_strings.clear();
}

} // namespace kphp_tracing
