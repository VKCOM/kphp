// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/component/inter-component-session/client.h"

inline constexpr std::string_view COMPONENT_NAME{"web"};

inline Optional<array<int64_t>> f$curl_multi_info_read(int64_t /*unused*/,
                                                       int64_t& /*unused*/ = CurlInstanceState::get().curl_multi_info_read_msgs_in_queue_stub) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}

inline bool f$curl_setopt_array(int64_t /*unused*/, const array<mixed>& /*unused*/) noexcept {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}


inline auto f$curl_exec(int64_t easy_id) noexcept -> kphp::coro::task<mixed> {
  auto c{kphp::component::InterComponentSession::Client::create(COMPONENT_NAME)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> data{static_cast <std::byte>(1), static_cast <std::byte>(2), static_cast <std::byte>(3)};
  std::span<const std::byte> req = data;
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp;
  co_await c.value().query(req, resp);

  co_return mixed{};
}
