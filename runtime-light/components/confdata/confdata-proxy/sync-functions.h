// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include "common/wrappers/overloaded.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/components/confdata/confdata-proxy/tl.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/rpc/rpc-query.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::confdata {

struct pagination {
  kphp::stl::string<kphp::memory::script_allocator> m_page;
  int64_t m_offset{};
  bool m_has_synced{};
};

enum class subscribe_error : uint8_t { transport, old_offset, malformed_response, not_synced, batch_rejected };

namespace details {

// Performs a single confdata.subscribe round-trip.
// On success, invokes `event_handler(events)` once with the batch of received events and updates `to` pagination.
// If the handler returns false, the batch is rejected with `batch_rejected` and `to` is left unchanged so it can be requested again.
// The batch is a view into the response buffer and is only valid for the duration of the call; empty batches are not delivered.
// An empty event value means that the key has been deleted.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto subscribe(std::string_view confdata_proxy_actor, kphp::confdata::pagination& to,
               const event_handler_type& event_handler) noexcept -> kphp::coro::task<std::expected<void, kphp::confdata::subscribe_error>> {
  // subscribe is a longpoll method, so the timeout must cover the time confdata-proxy may hold the request open
  static constexpr auto SUBSCRIBE_TIMEOUT{std::chrono::milliseconds{45'000}};

  const tl::RpcDestActorFlags<tl::confdata::Subscribe> request{.inner = {.actor_id = {},
                                                                         .flags = {.value = tl::rpcInvokeReqExtra::CUSTOM_TIMEOUT_MS_FLAG},
                                                                         .extra = {.opt_custom_timeout_ms = tl::i32{.value = SUBSCRIBE_TIMEOUT.count()}},
                                                                         .query = tl::confdata::Subscribe{
                                                                             .fields_mask = {},
                                                                             .access_token = {},
                                                                             .page = {.value = to.m_page},
                                                                             .offset = {.value = to.m_offset},
                                                                             .has_synced = {.value = to.m_has_synced},
                                                                             .prefixes = {.value = {{/* a single empty prefix subscribes to all keys */}}},

                                                                         }}};
  tl::storer tls{request.footprint()};
  request.store(tls);

  // client-side timeout must outlive the server-side longpoll (SUBSCRIBE_TIMEOUT); 10x is a safe margin
  auto expected_query{kphp::rpc::query::send(confdata_proxy_actor, SUBSCRIBE_TIMEOUT * 10, tls.view(), k2::RpcKind::TL_RPC)};
  if (!expected_query) [[unlikely]] {
    kphp::log::warning("confdata: failed to send subscribe request: {}", expected_query.error());
    co_return std::unexpected{kphp::confdata::subscribe_error::transport};
  }

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response_buffer{};
  auto expected_response{co_await kphp::rpc::query::response(std::move(*expected_query), [&response_buffer](size_t size) noexcept -> std::span<std::byte> {
    response_buffer.resize(size);
    return {response_buffer.data(), response_buffer.size()};
  })};
  if (!expected_response) [[unlikely]] {
    kphp::log::warning("confdata: failed to fetch subscribe response: {}", expected_response.error());
    co_return std::unexpected{kphp::confdata::subscribe_error::transport};
  }

  tl::fetcher tlf{*expected_response};
  tl::confdata::SubscribeResponse response{};
  if (!response.fetch(tlf)) [[unlikely]] {
    kphp::log::warning("confdata: failed to parse subscribe response");
    co_return std::unexpected{kphp::confdata::subscribe_error::malformed_response};
  }

  co_return std::visit(
      overloaded{
          [&event_handler, &to](const tl::confdata::subscribeResponseOk& response) noexcept -> std::expected<void, kphp::confdata::subscribe_error> {
            if (const auto& events{response.events}; events.size() != 0) {
              if (!std::invoke(event_handler, std::span<const tl::confdata::KeyValuePair>{events.value})) {
                return std::unexpected{kphp::confdata::subscribe_error::batch_rejected};
              }
            }

            to.m_page = response.new_page.value;
            to.m_offset = response.new_offset.value;
            to.m_has_synced = response.new_has_synced.value;
            return {};
          },
          [](const tl::confdata::subscribeResponseOldOffsetError& /* unused */) noexcept -> std::expected<void, kphp::confdata::subscribe_error> {
            return std::unexpected{kphp::confdata::subscribe_error::old_offset};
          },
      },
      response.value);
}

} // namespace details

// Paginates through a consistent snapshot of all subscribed keys until it has been fully synced.
// Returns the final pagination that should be passed to `update`.
//
// `event_handler` is invoked once per round-trip with a batch of events; the batch is only valid
// for the duration of the call and must be copied if it needs to be retained.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto sync(std::string_view confdata_proxy_actor,
          event_handler_type event_handler) noexcept -> kphp::coro::task<std::expected<kphp::confdata::pagination, kphp::confdata::subscribe_error>> {
  kphp::confdata::pagination p{};
  for (; !p.m_has_synced;) {
    if (auto expected{co_await details::subscribe(confdata_proxy_actor, p, event_handler)}; !expected) [[unlikely]] {
      co_return std::unexpected{expected.error()};
    }
  }
  co_return std::move(p);
}

// Longpoll loop: invokes `event_handler` for each event as it arrives, throttled to at most one batch per second:
// events that arrive between round-trips are buffered by the proxy and coalesced into the next batch. Returns only on error;
// `subscribe_error::old_offset` means that the local version is too old and a clean `sync` is required.
// `from` must be a synced pagination, typically the one returned by `sync`; `subscribe_error::not_synced` is returned otherwise.
//
// `event_handler` is invoked once per round-trip with a batch of events; the batch is only valid
// for the duration of the call and must be copied if it needs to be retained.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto update(std::string_view confdata_proxy_actor, kphp::confdata::pagination& from,
            event_handler_type event_handler) noexcept -> kphp::coro::task<std::expected<void, kphp::confdata::subscribe_error>> {
  // limits the update rate to at most one batch per interval
  static constexpr auto UPDATE_INTERVAL{std::chrono::seconds{1}};

  if (!from.m_has_synced) [[unlikely]] {
    co_return std::unexpected{kphp::confdata::subscribe_error::not_synced};
  }

  for (;;) {
    if (auto expected{co_await details::subscribe(confdata_proxy_actor, from, event_handler)}; !expected) [[unlikely]] {
      co_return std::unexpected{expected.error()};
    }
    co_await kphp::coro::io_scheduler::get().schedule(UPDATE_INTERVAL);
  }
}

} // namespace kphp::confdata
