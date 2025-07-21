// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/component/component-api.h"

#include <cstddef>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/utils/logs.h"

// === component query client interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_client_send_request(string name, string message) noexcept {
  auto expected{kphp::component::stream::open({name.c_str(), name.size()}, k2::stream_kind::component)};
  if (!expected) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }

  auto stream{std::move(*expected)};
  if (auto expected{co_await kphp::forks::id_managed(stream.write({reinterpret_cast<const std::byte*>(message.c_str()), message.size()}))}; !expected)
      [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }
  stream.shutdown_write();
  co_return make_instance<C$ComponentQuery>(std::move(stream));
}

kphp::coro::task<string> f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't fetch component response from null query");
    co_return string{};
  }

  auto& stream{query.get()->stream()};
  stream.clear();
  if (auto expected{co_await kphp::forks::id_managed(stream.read())}; !expected) [[unlikely]] {
    co_return string{};
  }
  co_return string{reinterpret_cast<const char*>(stream.data().data()), static_cast<string::size_type>(stream.data().size())};
}

// === component query server interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_server_accept_query() noexcept {
  auto opt_stream{co_await kphp::forks::id_managed(kphp::component::stream::accept())};
  if (!opt_stream) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }
  co_return make_instance<C$ComponentQuery>(std::move(*opt_stream));
}

kphp::coro::task<string> f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't fetch server request from null query");
    co_return string{};
  }

  auto& stream{query.get()->stream()};
  stream.clear();
  if (auto expected{co_await kphp::forks::id_managed(stream.read())}; !expected) [[unlikely]] {
    co_return string{};
  }
  co_return string{reinterpret_cast<const char*>(stream.data().data()), static_cast<string::size_type>(stream.data().size())};
}

kphp::coro::task<> f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't send server response to null query");
    co_return;
  }

  auto& stream{query.get()->stream()};
  co_await kphp::forks::id_managed(stream.write({reinterpret_cast<const std::byte*>(message.c_str()), message.size()}));
}
