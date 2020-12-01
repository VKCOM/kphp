// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/wrappers/likely.h"
#include "server/json-logger.h"

namespace {

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

template<class Mapper>
JsonLogger::JsonBuffer &JsonLogger::JsonBuffer::append_string_safe(vk::string_view value, Mapper value_mapper) noexcept {
  *last_++ = '"';
  constexpr size_t reserved_bytes = 16;
  size_t left_bytes = buffer_.size() - static_cast<size_t>(last_ - buffer_.data());
  assert(left_bytes >= reserved_bytes);
  left_bytes -= reserved_bytes;
  const size_t write_bytes = std::min(left_bytes, value.size());
  std::transform(value.begin(), value.begin() + write_bytes, last_, value_mapper);
  last_ += write_bytes;
  if (unlikely(write_bytes != value.size())) {
    const vk::string_view truncated_dots{"..."};
    std::copy(truncated_dots.begin(), truncated_dots.end(), last_);
    last_ += truncated_dots.size();
  }
  *last_++ = '"';
  *last_++ = ',';
  return *this;
}

JsonLogger &JsonLogger::get() noexcept {
  static JsonLogger logger;
  return logger;
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
  copy_if_enough_size(env, env_, env_buffer_, env_available_);
}

void JsonLogger::write_log(vk::string_view message, int type, int64_t created_at, void *const *trace, int64_t trace_size, bool uncaught_exception) noexcept {
  if (json_log_fd_ <= 0) {
    return;
  }

  auto json_out_it = buffers_.begin();
  for (; json_out_it != buffers_.end() && !json_out_it->try_start_json(); ++json_out_it) {
  }
  assert(json_out_it != buffers_.end());

  json_out_it->append_key("version").append_integer(release_version_);
  json_out_it->append_key("type").append_integer(type);
  json_out_it->append_key("created_at").append_integer(created_at);
  json_out_it->append_key("env").append_string(env_available_ ? env_ : vk::string_view{});

  if (uncaught_exception || tags_available_) {
    json_out_it->append_key("tags").start<'{'>();
    if (uncaught_exception) {
      json_out_it->append_raw(R"json("uncaught":true)json");
    }
    if (tags_available_) {
      json_out_it->append_raw(tags_);
    }
    json_out_it->finish<'}'>();
  }
  if (extra_info_available_) {
    json_out_it->append_key("extra_info").start<'{'>().append_raw(extra_info_).finish<'}'>();
  }

  json_out_it->append_key("trace").start<'['>();
  for (int64_t i = 0; i < trace_size; i++) {
    json_out_it->append_hex_as_string(reinterpret_cast<int64_t>(trace[i]));
  }
  json_out_it->finish<']'>();

  json_out_it->append_key("msg").append_string_safe(message, [](char c) {
    return c == '"' ? '\'' : (c == '\n' ? ' ' : c);
  });
  json_out_it->finish_json_and_flush(json_log_fd_);
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
