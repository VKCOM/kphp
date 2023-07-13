// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstring>
#include <cinttypes>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/fast-backtrace.h"
#include "common/wrappers/likely.h"
#include "runtime/kphp-backtrace.h"
#include "server/server-config.h"
#include "server/json-logger.h"
#include "server/php-engine-vars.h"

namespace {

enum {
  ServerLogCritical = -1,
  ServerLogError = -2,
  ServerLogWarning = -3
};

template<size_t N>
void copy_if_enough_size(vk::string_view src, vk::string_view &dest, std::array<char, N> &buffer, volatile std::atomic<bool> &availability_flag) noexcept {
  if (src.size() <= buffer.size()) {
    availability_flag = false;
    std::copy(src.begin(), src.end(), buffer.begin());
    dest = {buffer.data(), src.size()};
    availability_flag = !dest.empty();
  }
}

template<uint8_t BASE>
char *append_raw_integer(char *buffer, int64_t value) noexcept {
  static_assert(vk::any_of_equal(BASE, 10, 16), "Expected 10 or 16");
  char *first = buffer;
  auto unsigned_integer = static_cast<uint64_t>(value);
  if (BASE == 16) {
    *first++ = '0';
    *first++ = 'x';
  } else if (value < 0) {
    *first++ = '-';
    unsigned_integer = ~unsigned_integer + 1;
  }
  char *last = first;
  do {
    const uint64_t c = unsigned_integer % BASE;
    unsigned_integer /= BASE;
    *last++ = (BASE == 16 && c >= 10) ? static_cast<char>('a' + c - 10) : static_cast<char>('0' + c);
  } while (unsigned_integer);
  std::reverse(first, last);
  return last;
}

bool copy_raw_string(char *&out, size_t out_size, vk::string_view str) noexcept {
  size_t i = 0;
  for (; i != str.size() && out_size; ++i, --out_size) {
    const char c = str[i];
    if (std::iscntrl(c)) {
      *out++ = std::isspace(c) ? ' ' : '?';
    } else {
      if (vk::any_of_equal(c, '/', '\\', '"')) {
        if (!--out_size) {
          return false;
        }
        *out++ = '\\';
      }
      *out++ = c;
    }
  }
  return i == str.size();
}

} // namespace

bool JsonLogger::JsonBuffer::try_start_json() noexcept {
  const bool acquired = !busy_.exchange(true);
  if (acquired) {
    last_ = buffer_.begin();
    *last_++ = '{';
  }
  return acquired;
}

void JsonLogger::JsonBuffer::finish_json_and_flush(int out_fd) noexcept {
  assert(*(last_ - 1) == ',');
  *(last_ - 1) = '}';
  *last_++ = '\n';
  const auto r = write(out_fd, buffer_.data(), static_cast<size_t>(last_ - buffer_.data()));
  // TODO assert?
  static_cast<void>(r);
  force_reset();
}

void JsonLogger::JsonBuffer::force_reset() noexcept {
  last_ = nullptr;
  busy_ = false;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_key(vk::string_view key) noexcept {
  *last_++ = '"';
  std::copy(key.begin(), key.end(), last_);
  last_ += key.size();
  *last_++ = '"';
  *last_++ = ':';
  return *this;
}

template<char BRACKET>
JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::start() noexcept {
  *last_++ = BRACKET;
  return *this;
}

template<char BRACKET>
JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::finish() noexcept {
  if (*(last_ - 1) == ',') {
    *(last_ - 1) = BRACKET;
  } else {
    *last_++ = BRACKET;
  }
  *last_++ = ',';
  return *this;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_string(vk::string_view value) noexcept {
  *last_++ = '"';
  std::copy(value.begin(), value.end(), last_);
  last_ += value.size();
  *last_++ = '"';
  *last_++ = ',';
  return *this;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_raw(vk::string_view raw_element) noexcept {
  std::copy(raw_element.begin(), raw_element.end(), last_);
  last_ += raw_element.size();
  *last_++ = ',';
  return *this;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_integer(int64_t value) noexcept {
  last_ = append_raw_integer<10>(last_, value);
  *last_++ = ',';
  return *this;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_hex_as_string(int64_t value) noexcept {
  *last_++ = '"';
  last_ = append_raw_integer<16>(last_, value);
  *last_++ = '"';
  *last_++ = ',';
  return *this;
}

JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_raw_string(vk::string_view value) noexcept {
  *last_++ = '"';
  constexpr size_t reserved_bytes = 16;
  const size_t left_bytes = buffer_.size() - static_cast<size_t>(last_ - buffer_.data());
  assert(left_bytes >= reserved_bytes);
  if (unlikely(!copy_raw_string(last_, left_bytes - reserved_bytes, value))) {
    const vk::string_view truncated_dots{"..."};
    std::copy(truncated_dots.begin(), truncated_dots.end(), last_);
    last_ += truncated_dots.size();
  }
  *last_++ = '"';
  *last_++ = ',';
  return *this;
}

void JsonLogger::init(int64_t release_version) noexcept {
  release_version_ = release_version;
}

bool JsonLogger::reopen_log_file(const char *log_file_name) noexcept {
  if (json_log_fd_ > 0) {
    close(json_log_fd_);
  }
  json_log_fd_ = open(log_file_name, O_WRONLY | O_APPEND | O_CREAT, 0640);
  return json_log_fd_ > 0;
}

bool JsonLogger::reopen_traces_file(const char *traces_file_name) noexcept {
  if (traces_log_fd_ > 0) {
    close(traces_log_fd_);
  }
  traces_log_fd_ = open(traces_file_name, O_WRONLY | O_APPEND | O_CREAT, 0640);
  return traces_log_fd_ > 0;
}

void JsonLogger::fsync_log_file() const noexcept {
  if (json_log_fd_ > 0) {
    fsync(json_log_fd_);
  }
}

void JsonLogger::set_tags(vk::string_view tags) noexcept {
  copy_if_enough_size(tags, tags_, tags_buffer_, tags_available_);
}

void JsonLogger::set_extra_info(vk::string_view extra_info) noexcept {
  copy_if_enough_size(extra_info, extra_info_, extra_info_buffer_, extra_info_available_);
}

void JsonLogger::set_env(vk::string_view env) noexcept {
  char *out = env_buffer_.data();
  env_available_ = false;
  if (copy_raw_string(out, env_buffer_.size() - 1, env)) {
    *out = '\0';
    env_ = {env_buffer_.data(), static_cast<size_t>(out - env_buffer_.data())};
    env_available_ = true;
  }
}

void JsonLogger::write_log_with_demangled_backtrace(vk::string_view message,int type, int64_t created_at,
                                                    void *const *trace, int64_t trace_size, bool uncaught) {
  if (json_log_fd_ <= 0) {
    return;
  }

  auto *json_out_it = buffers_.begin();
  for (; json_out_it != buffers_.end() && !json_out_it->try_start_json(); ++json_out_it) {
  }
  assert(json_out_it != buffers_.end());

  write_general_info(json_out_it, type, created_at, uncaught);

  KphpBacktrace demangler{trace, static_cast<int32_t>(trace_size)};
  json_out_it->append_key("trace").start<'['>();
  for (const char *name : demangler.make_demangled_backtrace_range()) {
    if (name && strcmp(name, "") != 0) {
      json_out_it->append_raw_string(name);
    }
  }
  json_out_it->finish<']'>();

  json_out_it->append_key("msg").append_raw_string(message);
  json_out_it->finish_json_and_flush(json_log_fd_);

  json_logs_count = json_logs_count + 1;
}

void JsonLogger::write_log(vk::string_view message, int type, int64_t created_at,
                           void *const *trace, int64_t trace_size, bool uncaught) noexcept {
  if (json_log_fd_ <= 0) {
    return;
  }

  auto *json_out_it = buffers_.begin();
  for (; json_out_it != buffers_.end() && !json_out_it->try_start_json(); ++json_out_it) {
  }
  assert(json_out_it != buffers_.end());

  write_general_info(json_out_it, type, created_at, uncaught);

  json_out_it->append_key("trace").start<'['>();
  for (int64_t i = 0; i < trace_size; i++) {
    json_out_it->append_hex_as_string(reinterpret_cast<int64_t>(trace[i]));
  }
  json_out_it->finish<']'>();

  json_out_it->append_key("msg").append_raw_string(message);
  json_out_it->finish_json_and_flush(json_log_fd_);

  json_logs_count = json_logs_count + 1;
}

void JsonLogger::write_trace_line(const char *json_buf, size_t buf_len) noexcept {
  if (traces_log_fd_ <= 0) {
    return;
  }

  write(traces_log_fd_, json_buf, buf_len);
  json_traces_count = json_traces_count + 1;
}

void JsonLogger::write_log_with_backtrace(vk::string_view message, int type) noexcept {
  std::array<void *, 64> trace{};
  const int trace_size = backtrace(trace.data(), trace.size());
  write_log(message, type, time(nullptr), trace.data(), trace_size, true);
}

void JsonLogger::write_stack_overflow_log(int type) noexcept {
  std::array<void *, 64> trace{};
  const int trace_size = fast_backtrace_without_recursions(trace.data(), trace.size());
  write_log("Stack overflow", type, time(nullptr), trace.data(), trace_size, true);
}

void JsonLogger::reset_buffers() noexcept {
  tags_available_ = false;
  extra_info_available_ = false;
  env_available_ = false;
  tags_ = {};
  extra_info_ = {};
  env_ = {};
  for (auto &buffer: buffers_) {
    buffer.force_reset();
  }
}

void JsonLogger::write_general_info(JsonBuffer *json_out_it, int type, int64_t created_at, bool uncaught) {
  json_out_it->append_key("version").append_integer(release_version_);
  json_out_it->append_key("type").append_integer(type);
  json_out_it->append_key("created_at").append_integer(created_at);
  json_out_it->append_key("env").append_string(env_available_ ? env_ : vk::string_view{});

  json_out_it->append_key("tags").start<'{'>();
  if (tags_available_) {
    json_out_it->append_raw(tags_);
  }
  if (process_type == ProcessType::master) {
    json_out_it->append_key("process_type").append_string("master");
  } else if (process_type == ProcessType::http_worker) {
    json_out_it->append_key("process_type").append_string("http_worker");
    json_out_it->append_key("logname_id").append_integer(logname_id);
  } else if (process_type == ProcessType::rpc_worker) {
    json_out_it->append_key("process_type").append_string("rpc_worker");
    json_out_it->append_key("logname_id").append_integer(logname_id);
  } else if (process_type == ProcessType::job_worker) {
    json_out_it->append_key("process_type").append_string("job_worker");
    json_out_it->append_key("logname_id").append_integer(logname_id);
  }
  json_out_it->append_key("pid").append_integer(pid);
  json_out_it->append_key("cluster").append_string(vk::singleton<ServerConfig>::get().get_cluster_name());
  json_out_it->append_raw(uncaught ? R"json("uncaught":true)json" : R"json("uncaught":false)json");
  json_out_it->finish<'}'>();

  if (extra_info_available_) {
    json_out_it->append_key("extra_info").start<'{'>().append_raw(extra_info_).finish<'}'>();
  }
}

