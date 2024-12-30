// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/component/component-api.h"

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"

// === component query client interface ===========================================================

task_t<class_instance<C$ComponentQuery>> f$component_client_send_request(string name, string message) noexcept {
  const auto [stream_d, errc]{InstanceState::get().open_stream({name.c_str(), name.size()}, k2::stream_kind::component)};
  if (errc != k2::errno_ok) {
    co_return class_instance<C$ComponentQuery>{};
  }

  int32_t written{co_await write_all_to_stream(stream_d, message.c_str(), message.size())};
  if (written != message.size()) {
    php_warning("can't send request to component '%s'", name.c_str());
    co_return class_instance<C$ComponentQuery>{};
  }

  k2::shutdown_write(stream_d);
  php_debug("sent %d bytes from %d to '%s' on stream %" PRIu64, written, message.size(), name.c_str(), stream_d);
  co_return make_instance<C$ComponentQuery>(stream_d);
}

task_t<string> f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  if (stream_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
    php_warning("can't fetch component response from stream %" PRIu64, stream_d);
    co_return string{};
  }

  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer, static_cast<string::size_type>(size)};
  k2::free(buffer);
  php_debug("read %d bytes from stream %" PRIu64, size, stream_d);
  InstanceState::get().release_stream(stream_d);
  query.get()->stream_d = k2::INVALID_PLATFORM_DESCRIPTOR;
  co_return result;
}

// === component query server interface ===========================================================

task_t<class_instance<C$ComponentQuery>> f$component_server_accept_query() noexcept {
  co_return make_instance<C$ComponentQuery>(co_await wait_for_incoming_stream_t{});
}

task_t<string> f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer, static_cast<string::size_type>(size)};
  k2::free(buffer);
  co_return result;
}

task_t<void> f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept {
  uint64_t stream_d{query.is_null() ? k2::INVALID_PLATFORM_DESCRIPTOR : query.get()->stream_d};
  if ((co_await write_all_to_stream(stream_d, message.c_str(), message.size())) != message.size()) {
    php_warning("can't send component response to stream %" PRIu64, stream_d);
  } else {
    php_debug("sent %d bytes as response to stream %" PRIu64, message.size(), stream_d);
  }
  InstanceState::get().release_stream(stream_d);
}
