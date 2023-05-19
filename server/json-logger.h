// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <array>
#include <atomic>
#include <csignal>

#include "common/functional/identity.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/wrappers/string_view.h"


class JsonLogger : vk::not_copyable {
public:
  friend class vk::singleton<JsonLogger>;

  void init(int64_t release_version) noexcept;

  bool reopen_log_file(const char *log_file_name) noexcept;
  bool reopen_traces_file(const char *traces_file_name) noexcept;

  void fsync_log_file() const noexcept;

  void set_tags(vk::string_view tags) noexcept;
  void set_extra_info(vk::string_view extra_info) noexcept;
  void set_env(vk::string_view env) noexcept;

  uint64_t get_json_logs_count() const noexcept {
    return json_logs_count;
  }

  uint64_t get_json_traces_count() const noexcept {
    return json_traces_count;
  }

  void reset_json_logs_count() noexcept {
    json_logs_count = 0;
    json_traces_count = 0;
  }

  // ATTENTION: this function isn't signal-safety and cannot be used inside signal handlers
  void write_log_with_demangled_backtrace(vk::string_view message, int type, int64_t created_at, void *const *trace, int64_t trace_size, bool uncaught);

  // ATTENTION: this functions are used in signal handlers, therefore they are expected to be safe for them
  // Details: https://man7.org/linux/man-pages/man7/signal-safety.7.html
  // todo: functions bellow use backtrace which isn't async-signal safety
  void write_log(vk::string_view message, int type, int64_t created_at, void *const *trace, int64_t trace_size, bool uncaught) noexcept;
  void write_log_with_backtrace(vk::string_view message, int type) noexcept;
  void write_stack_overflow_log(int type) noexcept;

  void write_trace_line(const char *json_buf, size_t buf_len) noexcept;

  void reset_buffers() noexcept;

private:
  JsonLogger() = default;

  // it's incremented from signal handler sometimes => it must be volatile sig_atomic_t to prevent data races
  volatile sig_atomic_t json_logs_count{0};
  volatile sig_atomic_t json_traces_count{0};

  int64_t release_version_{0};
  int json_log_fd_{-1};   // kphp_log.json
  int traces_log_fd_{-1}; // kphp_log.traces.jsonl

#if __cplusplus >= 201703
  static_assert(std::atomic<bool>::is_always_lock_free);
#endif

  // This flags help avoid of undefined state of corresponding strings in signal handlers
  volatile std::atomic<bool> tags_available_{false};
  volatile std::atomic<bool> extra_info_available_{false};
  volatile std::atomic<bool> env_available_{false};

  vk::string_view tags_;
  vk::string_view extra_info_;
  vk::string_view env_;

  std::array<char, 10 * 1024> tags_buffer_{{0}};
  std::array<char, 10 * 1024> extra_info_buffer_{{0}};
  std::array<char, 128> env_buffer_{{0}};

  class JsonBuffer : vk::not_copyable {
  public:
    bool try_start_json() noexcept;
    void finish_json_and_flush(int out_fd) noexcept;
    JsonBuffer &append_key(vk::string_view key) noexcept;
    template<char BRACKET>
    JsonBuffer &start() noexcept;
    template<char BRACKET>
    JsonBuffer &finish() noexcept;
    JsonBuffer &append_string(vk::string_view value) noexcept;
    JsonBuffer &append_raw(vk::string_view raw_element) noexcept;
    JsonBuffer &append_integer(int64_t value) noexcept;
    JsonBuffer &append_hex_as_string(int64_t value) noexcept;
    JsonBuffer &append_raw_string(vk::string_view raw_value) noexcept;

    void force_reset() noexcept;

  private:
    // This flag shows that buffer is used right now,
    // it is used if signal is raised during the buffer filling
    volatile std::atomic<bool> busy_{false};
    char *last_{nullptr};
    std::array<char, 32 * 1024> buffer_{{0}};
  };
  std::array<JsonBuffer, 8> buffers_;

  void write_general_info(JsonBuffer * json_out_it, int type, int64_t created_at, bool uncaught);
};

