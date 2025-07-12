// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/streams/stream.h"

using resource = mixed;

enum class resource_kind : uint8_t { STDIN, STDOUT, STDERR, INPUT, UDP, UNKNOWN };

namespace resource_impl_ {

inline constexpr std::string_view STDIN_NAME = "php://stdin";
inline constexpr std::string_view STDOUT_NAME = "php://stdout";
inline constexpr std::string_view STDERR_NAME = "php://stderr";
inline constexpr std::string_view INPUT_NAME = "php://input";
inline constexpr std::string_view UDP_SCHEME_PREFIX = "udp://";

inline resource_kind uri_to_resource_kind(std::string_view uri) noexcept {
  resource_kind kind{resource_kind::UNKNOWN};
  if (uri == resource_impl_::STDIN_NAME) {
    kind = resource_kind::STDIN;
  } else if (uri == resource_impl_::STDOUT_NAME) {
    kind = resource_kind::STDOUT;
  } else if (uri == resource_impl_::STDERR_NAME) {
    kind = resource_kind::STDERR;
  } else if (uri == resource_impl_::INPUT_NAME) {
    kind = resource_kind::INPUT;
  } else if (uri.starts_with(resource_impl_::UDP_SCHEME_PREFIX)) {
    kind = resource_kind::UDP;
  }
  return kind;
}

} // namespace resource_impl_

class underlying_resource_t : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  std::optional<kphp::component::stream> m_opt_stream;

public:
  resource_kind kind{resource_kind::UNKNOWN};
  int32_t last_errc{k2::errno_ok};

  explicit underlying_resource_t(std::string_view) noexcept;
  underlying_resource_t(underlying_resource_t&& other) noexcept;
  ~underlying_resource_t() override;

  underlying_resource_t(const underlying_resource_t&) = delete;
  underlying_resource_t& operator=(const underlying_resource_t&) = delete;
  underlying_resource_t& operator=(underlying_resource_t&&) = delete;

  const char* get_class() const noexcept override {
    return R"(resource)";
  }

  kphp::coro::task<int64_t> write(std::string_view text) noexcept {
    if (kind == resource_kind::STDERR) {
      co_return k2::stderr_write(text.size(), text.data());
    }

    if (!m_opt_stream.has_value()) [[unlikely]] {
      co_return (last_errc = k2::errno_einval, 0);
    }

    const auto& stream{*m_opt_stream};
    if (auto expected{co_await stream.write({reinterpret_cast<const std::byte*>(text.data()), text.size()})}; !expected) {
      co_return (last_errc = expected.error(), 0);
    }
    co_return text.size();
  }

  Optional<string> get_contents() const noexcept {
    auto& http_server_instance_st{HttpServerInstanceState::get()};
    if (kind != resource_kind::INPUT || !http_server_instance_st.opt_raw_post_data.has_value()) {
      return false;
    }
    return *http_server_instance_st.opt_raw_post_data;
  }

  void flush() const noexcept {}

  void close() noexcept;
};
