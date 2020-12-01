// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <array>
#include <atomic>

#include "common/functional/identity.h"
#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"


class JsonLogger : vk::not_copyable {
public:
  static JsonLogger &get() noexcept;

  bool reopen_log_file(const char *log_file_name) noexcept;

  void fsync_log_file() noexcept;

  void set_tags(vk::string_view tags) noexcept;
  void set_extra_info(vk::string_view extra_info) noexcept;
  void set_env(vk::string_view env) noexcept;

  void write_log(vk::string_view message, int version, int type, void **trace, int trace_size) noexcept;
  void reset_buffers() noexcept;

private:
  JsonLogger() = default;

  int json_log_fd_{-1};

  vk::string_view tags_;
  vk::string_view extra_info_;
  vk::string_view env_;

  std::array<char, 10 * 1024> tags_buffer_{0};
  std::array<char, 10 * 1024> extra_info_buffer_{0};
  std::array<char, 128> env_buffer_{0};

  class JsonBuffer : vk::not_copyable {
  public:
    bool try_start_json() noexcept;
    void finish_json_and_flush(int out_fd) noexcept;
    JsonBuffer &append_key(vk::string_view key) noexcept;
    JsonBuffer &start_array() noexcept;
    JsonBuffer &finish_array() noexcept;
    JsonBuffer &append_string(vk::string_view value) noexcept;
    JsonBuffer &append_raw(vk::string_view raw_element) noexcept;
    JsonBuffer &append_integer(int64_t value) noexcept;
    JsonBuffer &append_hex_as_string(int64_t value) noexcept;
    template<class Mapper = vk::identity>
    JsonBuffer &append_string_safe(vk::string_view value, Mapper value_mapper = {}) noexcept;

    void force_reset() noexcept;

  private:
#if __cplusplus >= 201703
  static_assert(std::atomic<bool>::is_always_lock_free);
#endif
    volatile std::atomic<bool> busy_{false};
    char *last_{nullptr};
    std::array<char, 32 * 1024> buffer_{0};
  };
  std::array<JsonBuffer, 8> buffers_;
};

