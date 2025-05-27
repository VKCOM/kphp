// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/stdlib/system/system-state.h"
#include "runtime-light/utils/logs.h"

template<typename F>
bool f$register_kphp_on_oom_callback(F&& /*callback*/) {
  kphp::log::error("call to unsupported function");
}

template<typename F>
void f$kphp_extended_instance_cache_metrics_init(F&& /*callback*/) {
  kphp::log::error("call to unsupported function");
}

inline int64_t f$system(const string& /*command*/, int64_t& /*result_code*/ = SystemInstanceState::get().result_code_dummy) {
  kphp::log::error("call to unsupported function");
}

inline Optional<array<mixed>> f$getopt(const string& /*options*/, const array<string>& /*longopts*/ = {},
                                       Optional<int64_t>& /*rest_index*/ = SystemInstanceState::get().rest_index_dummy) {
  kphp::log::error("call to unsupported function");
}

inline int64_t f$numa_get_bound_node() noexcept {
  return -1;
}

inline void f$kphp_set_context_on_error([[maybe_unused]] const array<mixed>& tags, [[maybe_unused]] const array<mixed>& extra_info,
                                        [[maybe_unused]] const string& env = {}) noexcept {}

inline int64_t f$posix_getpid() noexcept {
  return static_cast<int64_t>(ImageState::get().pid);
}

inline string f$php_uname(const string& mode = string{1, 'a'}) noexcept {
  const auto& image_st{ImageState::get()};
  const char mode_c{mode.empty() ? 'a' : mode[0]};
  switch (mode_c) {
  case 's':
    return image_st.uname_info_s;
  case 'n':
    return image_st.uname_info_n;
  case 'r':
    return image_st.uname_info_r;
  case 'v':
    return image_st.uname_info_v;
  case 'm':
    return image_st.uname_info_m;
  default:
    return image_st.uname_info_a;
  }
}

inline string f$php_sapi_name() noexcept {
  return PhpScriptMutableGlobals::current().get_superglobals().v$d$PHP_SAPI;
}

Optional<string> f$iconv(const string& input_encoding, const string& output_encoding, const string& input_str) noexcept;

inline kphp::coro::task<> f$usleep(int64_t microseconds) noexcept {
  if (microseconds <= 0) [[unlikely]] {
    kphp::log::warning("value of microseconds ({}) must be positive", microseconds);
    co_return;
  }
  const std::chrono::microseconds sleep_time{microseconds};
  co_await wait_for_timer_t{sleep_time};
}

inline array<array<string>> f$debug_backtrace() noexcept {
  kphp::log::warning("called stub debug_backtrace");
  return {};
}

inline int64_t f$error_reporting([[maybe_unused]] int64_t level) noexcept {
  kphp::log::warning("called stub error_reporting");
  return 0;
}

inline int64_t f$error_reporting() noexcept {
  kphp::log::warning("called stub error_reporting");
  return 0;
}

inline Optional<string> f$exec([[maybe_unused]] const string& command) noexcept {
  kphp::log::error("call to unsupported function");
}

inline Optional<string> f$exec([[maybe_unused]] const string& command, [[maybe_unused]] mixed& output,
                               [[maybe_unused]] int64_t& result_code = SystemInstanceState::get().result_code_dummy) noexcept {
  kphp::log::error("call to unsupported function");
}

inline string f$get_engine_version() noexcept {
  kphp::log::warning("called stub get_engine_version");
  return {};
}

inline string f$get_kphp_cluster_name() noexcept {
  kphp::log::warning("called stub get_kphp_cluster_name");
  return string{"adm512"};
}
