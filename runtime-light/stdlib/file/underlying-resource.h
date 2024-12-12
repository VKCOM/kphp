// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/streams/streams.h"

namespace resource_impl_ {

inline constexpr std::string_view STDIN_NAME = "php://stdin";
inline constexpr std::string_view STDOUT_NAME = "php://stdout";
inline constexpr std::string_view STDERR_NAME = "php://stderr";
inline constexpr std::string_view UDP_SCHEME_PREFIX = "udp://";

} // namespace resource_impl_

class underlying_resource_t : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  uint64_t stream_d_{k2::INVALID_PLATFORM_DESCRIPTOR};

public:
  enum class kind_t : uint8_t {
    in,
    out,
    err,
    udp,
    unknown,
  };

  kind_t kind{kind_t::unknown};
  int32_t last_error{k2::errno_ok};

  explicit underlying_resource_t(std::string_view) noexcept;
  underlying_resource_t(underlying_resource_t &&other) noexcept;
  ~underlying_resource_t() override;

  underlying_resource_t(const underlying_resource_t &) = delete;
  underlying_resource_t &operator=(const underlying_resource_t &) = delete;
  underlying_resource_t &operator=(underlying_resource_t &&) = delete;

  const char *get_class() const noexcept override {
    return R"(resource)";
  }

  task_t<int64_t> write(std::string_view text) const noexcept {
    co_return co_await write_all_to_stream(stream_d_, text.data(), text.size());
  }

  Optional<string> get_contents() const noexcept {
    auto &http_server_instance_st{HttpServerInstanceState::get()};
    if (kind != kind_t::in || !http_server_instance_st.opt_raw_post_data.has_value()) {
      return false;
    }
    return *http_server_instance_st.opt_raw_post_data;
  }

  void flush() const noexcept {}

  void close() noexcept;
};
