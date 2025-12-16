// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <pwd.h>
#include <ranges>
#include <span>
#include <string_view>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#include "common/algorithms/string-algorithms.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/stdlib/diagnostics/contextual-logger.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/system/system-state.h"

namespace kphp::posix::impl {

constexpr std::string_view NAME_PWUID_KEY = "name";
constexpr std::string_view PASSWD_PWUID_KEY = "passwd";
constexpr std::string_view UID_PWUID_KEY = "uid";
constexpr std::string_view GID_PWUID_KEY = "gid";
constexpr std::string_view GECOS_PWUID_KEY = "gecos";
constexpr std::string_view DIR_PWUID_KEY = "dir";
constexpr std::string_view SHELL_PWUID_KEY = "shell";

} // namespace kphp::posix::impl

namespace kphp::system {

template<std::invocable<std::span<std::byte>> output_handler_type = std::identity>
auto exec(std::string_view cmd, const output_handler_type& output_handler = {}) noexcept -> std::expected<int32_t, int32_t> {
  static constexpr std::string_view program{"sh"};
  static constexpr std::string_view arg1{"-c"};

  if (cmd.empty()) [[unlikely]] {
    kphp::log::warning("exec command must not be empty");
    return std::unexpected{k2::errno_einval};
  }

  std::array args{k2::CommandArg{.arg = arg1.data(), .arg_len = arg1.size()}, k2::CommandArg{.arg = cmd.data(), .arg_len = cmd.size()}};
  auto expected{
      k2::command(program, args, std::is_same_v<output_handler_type, std::identity> ? k2::CommandStdoutPolicy::NoCapture : k2::CommandStdoutPolicy::Capture)};
  if (!expected) [[unlikely]] {
    kphp::log::warning("error executing command: error code -> {}, cmd -> '{}'", expected.error(), cmd);
    return std::unexpected{expected.error()};
  }

  const auto [exit_code, output, output_len]{*std::move(expected)};
  if constexpr (!std::is_same_v<output_handler_type, std::identity>) {
    std::invoke(output_handler, std::span<std::byte>{output.get(), output_len});
  }
  return std::expected<int32_t, int32_t>{exit_code};
}

} // namespace kphp::system

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

inline void f$kphp_set_context_on_error(const array<mixed>& tags, const Optional<array<mixed>>& extra_info = {}, const Optional<string>& env = {}) noexcept {
  auto logger_opt{kphp::log::contextual_logger::try_get()};
  if (!logger_opt.has_value()) [[unlikely]] {
    return;
  }
  auto& logger{(*logger_opt).get()};

  static constexpr std::string_view EXTRA_TAGS_KEY = "tags";
  static constexpr std::string_view EXTRA_INFO_KEY = "extra_info";
  static constexpr std::string_view ENVIRONMENT_KEY = "env";

  auto& static_SB{RuntimeContext::get().static_SB.clean()};
  logger.remove_extra_tag(EXTRA_TAGS_KEY);
  if (impl_::JsonEncoder(JSON_FORCE_OBJECT, false).encode(tags, static_SB)) [[likely]] {
    logger.add_extra_tag(EXTRA_TAGS_KEY, {static_SB.buffer(), static_SB.size()});
  }
  static_SB.clean();

  logger.remove_extra_tag(EXTRA_INFO_KEY);
  if (extra_info.has_value() && impl_::JsonEncoder(JSON_FORCE_OBJECT, false).encode(extra_info.val(), static_SB)) [[likely]] {
    logger.add_extra_tag(EXTRA_INFO_KEY, {static_SB.buffer(), static_SB.size()});
  }
  static_SB.clean();

  logger.remove_extra_tag(ENVIRONMENT_KEY);
  if (env.has_value()) {
    logger.add_extra_tag(ENVIRONMENT_KEY, {env.val().c_str(), env.val().size()});
  }
}

inline int64_t f$posix_getpid() noexcept {
  return static_cast<int64_t>(ImageState::get().pid);
}

inline int64_t f$posix_getuid() noexcept {
  return static_cast<int64_t>(ImageState::get().uid);
}

inline Optional<array<mixed>> f$posix_getpwuid(int64_t user_id) noexcept {
  static constexpr int64_t DEFAULT_PASSWD_BUFFER_SIZE = 4096;

  int64_t passwd_max_buffer_size{ImageState::get().passwd_max_buffer_size.value_or(DEFAULT_PASSWD_BUFFER_SIZE)};
  passwd pwd{};
  passwd* pwd_result{nullptr};
  std::unique_ptr<std::byte, decltype(std::addressof(kphp::memory::script::free))> buffer{
      static_cast<std::byte*>(kphp::memory::script::alloc(passwd_max_buffer_size)), kphp::memory::script::free};

  int32_t error_code{k2::getpwuid_r(static_cast<uid_t>(user_id), std::addressof(pwd), std::span{buffer.get(), static_cast<size_t>(passwd_max_buffer_size)},
                                    std::addressof(pwd_result))};

  if (error_code != k2::errno_ok || pwd_result != std::addressof(pwd)) [[unlikely]] {
    return false;
  }

  array<mixed> result{array_size{7, false}};
  result.set_value(string{kphp::posix::impl::NAME_PWUID_KEY.data(), kphp::posix::impl::NAME_PWUID_KEY.size()}, string{pwd.pw_name});
  result.set_value(string{kphp::posix::impl::PASSWD_PWUID_KEY.data(), kphp::posix::impl::NAME_PWUID_KEY.size()}, string{pwd.pw_passwd});
  result.set_value(string{kphp::posix::impl::UID_PWUID_KEY.data(), kphp::posix::impl::UID_PWUID_KEY.size()}, static_cast<int64_t>(pwd.pw_uid));
  result.set_value(string{kphp::posix::impl::GID_PWUID_KEY.data(), kphp::posix::impl::GID_PWUID_KEY.size()}, static_cast<int64_t>(pwd.pw_gid));
  result.set_value(string{kphp::posix::impl::GECOS_PWUID_KEY.data(), kphp::posix::impl::GECOS_PWUID_KEY.size()}, string{pwd.pw_gecos});
  result.set_value(string{kphp::posix::impl::DIR_PWUID_KEY.data(), kphp::posix::impl::DIR_PWUID_KEY.size()}, string{pwd.pw_dir});
  result.set_value(string{kphp::posix::impl::SHELL_PWUID_KEY.data(), kphp::posix::impl::SHELL_PWUID_KEY.size()}, string{pwd.pw_shell});
  return result;
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
  co_await kphp::forks::id_managed(kphp::coro::io_scheduler::get().yield_for(std::chrono::microseconds{microseconds}));
}

inline kphp::coro::task<> f$sleep(int64_t seconds) noexcept {
  if (seconds <= 0 || seconds > 1800) {
    kphp::log::warning("wrong parameter seconds ({}) specified in function sleep, must be in seconds", seconds);
    co_return;
  }

  co_await f$usleep(seconds * 1'000'000);
  co_return;
}

inline Optional<string> f$exec(const string& cmd, mixed& output, std::optional<std::reference_wrapper<int64_t>> exit_code = {}) noexcept {
  string last_line{};
  const auto output_handler{[&last_line, &output_mixed = output](std::span<std::byte> output_bytes) noexcept {
    std::string_view output{reinterpret_cast<const char*>(output_bytes.data()), output_bytes.size()};
    // PHP doesn't include trailing whitespace
    if (!output.empty() && vk::is_ascii_whitespace(output.back())) {
      output.remove_suffix(1);
    }

    const auto pos{output.rfind('\n')};
    const auto last_line_view{vk::rstrip_ascii_whitespace(output.substr(pos == std::string_view::npos ? 0 : (pos + 1)))};
    last_line = {last_line_view.data(), static_cast<string::size_type>(last_line_view.size())};

    if (output_mixed.is_array()) {
      std::ranges::for_each(output | std::views::split('\n'), [&output_arr = output_mixed.as_array()](auto rng) noexcept {
        const auto line_view{vk::rstrip_ascii_whitespace(std::string_view{rng})};
        output_arr.emplace_back(string{line_view.data(), static_cast<string::size_type>(line_view.size())});
      });
    }
  }};

  auto expected{kphp::system::exec({cmd.c_str(), cmd.size()}, output_handler)};
  if (!expected) [[unlikely]] {
    return false;
  }

  if (exit_code) {
    (*exit_code).get() = static_cast<int64_t>(*expected);
  }

  return last_line;
}

inline Optional<string> f$exec(const string& cmd) noexcept {
  mixed output_arr{};
  return f$exec(cmd, output_arr);
}

inline string f$get_engine_version() noexcept {
  kphp::log::warning("called stub get_engine_version");
  return {};
}

inline string f$get_kphp_cluster_name() noexcept {
  kphp::log::warning("called stub get_kphp_cluster_name");
  return string{"adm512"};
}

inline void f$kphp_turn_on_host_tag_in_inner_statshouse_metrics_toggle() noexcept {}
