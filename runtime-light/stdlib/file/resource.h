// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/cli/cli-instance-state.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/streams/stream.h"

using resource = mixed;

namespace kphp::resource {

namespace detail {

inline constexpr std::string_view STDIN_NAME = "php://stdin";
inline constexpr std::string_view STDOUT_NAME = "php://stdout";
inline constexpr std::string_view STDERR_NAME = "php://stderr";
inline constexpr std::string_view INPUT_NAME = "php://input";
inline constexpr std::string_view UDP_SCHEME_PREFIX = "udp://";

} // namespace detail

enum class resource_kind : uint8_t { STDIN, STDOUT, STDERR, INPUT, UDP, UNKNOWN };

inline resource_kind uri_to_resource_kind(std::string_view uri) noexcept {
  resource_kind kind{resource_kind::UNKNOWN};
  if (uri == detail::STDIN_NAME) {
    kind = resource_kind::STDIN;
  } else if (uri == detail::STDOUT_NAME) {
    kind = resource_kind::STDOUT;
  } else if (uri == detail::STDERR_NAME) {
    kind = resource_kind::STDERR;
  } else if (uri == detail::INPUT_NAME) {
    kind = resource_kind::INPUT;
  } else if (uri.starts_with(detail::UDP_SCHEME_PREFIX)) {
    kind = resource_kind::UDP;
  }
  return kind;
}

class underlying_resource : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  int32_t m_errc{k2::errno_ok};
  resource_kind m_kind{resource_kind::UNKNOWN};
  std::optional<kphp::component::stream> m_stream;

public:
  explicit underlying_resource(std::string_view) noexcept;

  underlying_resource(underlying_resource&& other) noexcept
      : m_errc(std::exchange(other.m_errc, k2::errno_ok)),
        m_kind(std::exchange(other.m_kind, resource_kind::UNKNOWN)),
        m_stream(std::move(other.m_stream)) {}

  ~underlying_resource() override {
    close();
  }

  underlying_resource(const underlying_resource&) = delete;
  underlying_resource& operator=(const underlying_resource&) = delete;
  underlying_resource& operator=(underlying_resource&&) = delete;

  const char* get_class() const noexcept override {
    return R"(resource)";
  }

  int32_t error_code() const noexcept {
    return m_errc;
  }

  resource_kind kind() const noexcept {
    return m_kind;
  }

  void close() noexcept {
    m_stream.reset();
  }

  void flush() const noexcept {}

  Optional<string> get_contents() const noexcept {
    auto& http_server_instance_st{HttpServerInstanceState::get()};
    if (m_kind != resource_kind::INPUT || !http_server_instance_st.opt_raw_post_data.has_value()) {
      return false;
    }
    return *http_server_instance_st.opt_raw_post_data;
  }

  kphp::coro::task<int64_t> write(std::string_view data) noexcept {
    switch (m_kind) {
    case resource_kind::STDERR:
      co_return k2::stderr_write(data.size(), data.data());
    case resource_kind::UDP:
      if (!m_stream) [[unlikely]] {
        co_return (m_errc = k2::errno_einval, 0);
      }

      if (auto expected{co_await (*m_stream).write({reinterpret_cast<const std::byte*>(data.data()), data.size()})}; !expected) [[unlikely]] {
        co_return (m_errc = expected.error(), 0);
      }
      co_return data.size();
    case resource_kind::STDOUT: {
      auto& cli_instance_state{CLIInstanceInstance::get()};
      if (!cli_instance_state.output_stream) [[unlikely]] {
        co_return (m_errc = k2::errno_einval, 0);
      }

      if (auto expected{co_await (*cli_instance_state.output_stream).write({reinterpret_cast<const std::byte*>(data.data()), data.size()})}; !expected)
          [[unlikely]] {
        co_return (m_errc = expected.error(), 0);
      }
      co_return data.size();
    }
    default:
      co_return (m_errc = k2::errno_einval, 0);
    }
  }
};

} // namespace kphp::resource
