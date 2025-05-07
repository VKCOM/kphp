// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/component/component-api.h"

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/logs.h"

// === component query client interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_client_send_request(string name, string message) noexcept {
  const auto [stream_d, errc]{InstanceState::get().open_stream({name.c_str(), name.size()}, k2::stream_kind::component)};
  if (errc != k2::errno_ok) {
    co_return class_instance<C$ComponentQuery>{};
  }

  int32_t written{co_await write_all_to_stream(stream_d, message.c_str(), message.size())};
  if (written != message.size()) {
    kphp::log::warning("can't send request to component '{}'", name.c_str());
    co_return class_instance<C$ComponentQuery>{};
  }

  k2::shutdown_write(stream_d);
  kphp::log::debug("send {} bytes to '{}' on stream {}", written, name.c_str(), stream_d);
  co_return make_instance<C$ComponentQuery>(stream_d);
}

kphp::coro::task<string> f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  if (stream_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
    kphp::log::warning("can't fetch component response from stream {}", stream_d);
    co_return string{};
  }

  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  kphp::log::debug("read {} bytes from stream {}", size, stream_d);
  string result{buffer.get(), static_cast<string::size_type>(size)};
  InstanceState::get().release_stream(stream_d);
  query.get()->stream_d = k2::INVALID_PLATFORM_DESCRIPTOR;
  co_return result;
}

// === component query server interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_server_accept_query() noexcept {
  co_return make_instance<C$ComponentQuery>(co_await wait_for_incoming_stream_t{});
}

kphp::coro::task<string> f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer.get(), static_cast<string::size_type>(size)};
  co_return result;
}

kphp::coro::task<> f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  if ((co_await write_all_to_stream(stream_d, message.c_str(), message.size())) != message.size()) {
    kphp::log::warning("can't send component response to stream {}", stream_d);
  } else {
    kphp::log::debug("sent {} bytes on stream {}", message.size(), stream_d);
  }
  InstanceState::get().release_stream(stream_d);
}
