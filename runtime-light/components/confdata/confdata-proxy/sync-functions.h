// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include "common/wrappers/overloaded.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
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

using encoded_snapshot_page = kphp::stl::vector<std::byte, kphp::memory::script_allocator>;
using encoded_snapshot_pages = kphp::stl::vector<encoded_snapshot_page, kphp::memory::script_allocator>;

struct snapshot final {
  pagination m_pagination;
  encoded_snapshot_pages m_pages;
};

enum class subscribe_error : uint8_t { transport, old_offset, malformed_response, not_synced, batch_rejected, not_enough_memory };

namespace details {

inline auto has_memory_for_response_process(size_t response_size) noexcept -> bool {
  static constexpr size_t MIN_ENCODED_PAIR_SIZE{tl::confdata::KeyValuePair{}.footprint()};
  static constexpr size_t MAX_DECODED_BYTES_PER_ENCODED_BYTE{(sizeof(tl::confdata::KeyValuePair) + MIN_ENCODED_PAIR_SIZE - 1) / MIN_ENCODED_PAIR_SIZE};

  // While a response is being processed, two allocations are alive at once:
  //   1. the encoded response itself, `response_size` bytes in `encoded_snapshot_page`;
  //   2. the decoded events: `tl::vector<KeyValuePair>` reserves `count * sizeof(KeyValuePair)` bytes. `tl::string` is a
  //      zero-copy view into (1), so keys and values add nothing to it.
  //
  // (2) is the largest relative to (1) when every pair is the smallest possible one. A longer key or value enlarges the
  // encoded pair but not the decoded one, so (2) never exceeds `MAX_DECODED_BYTES_PER_ENCODED_BYTE * response_size`.
  static constexpr size_t MEMORY_BYTES_PER_RESPONSE_BYTE{1 + MAX_DECODED_BYTES_PER_ENCODED_BYTE};

  if (response_size > std::numeric_limits<size_t>::max() / MEMORY_BYTES_PER_RESPONSE_BYTE) [[unlikely]] {
    return false;
  }

  // `is_enough_memory_for` is a sufficient condition: if a single free block can hold the whole `MEMORY_BYTES_PER_RESPONSE_BYTE * response_size`,
  // the buffer and then the decoded events are carved out of it one after another. The converse does not hold (the allocator may
  // still succeed by defragmenting or by combining several blocks), so the check may reject a response that would have fit.
  return RuntimeAllocator::get().get_memory_resource().is_enough_memory_for(response_size * MEMORY_BYTES_PER_RESPONSE_BYTE);
}

} // namespace details

// Performs a single confdata.subscribe round-trip.
// On success, invokes `event_handler(events)` once with the batch of received events and updates `to` pagination.
// If the handler returns false, the batch is rejected with `batch_rejected` and `to` is left unchanged so it can be requested again.
// The batch is a view into the response buffer and is only valid for the duration of the call; empty batches are not delivered.
// An empty event value means that the key has been deleted.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto subscribe(std::string_view confdata_proxy_actor, kphp::confdata::pagination& to, const event_handler_type& event_handler,
               std::optional<std::reference_wrapper<encoded_snapshot_page>> retained_response = {}) noexcept
    -> kphp::coro::task<std::expected<void, kphp::confdata::subscribe_error>> {
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
    kphp::log::warning("failed to send subscribe request: {}", expected_query.error());
    co_return std::unexpected{kphp::confdata::subscribe_error::transport};
  }

  encoded_snapshot_page response_buffer{};
  std::optional<size_t> rejected_response_size{};
  auto expected_response{co_await kphp::rpc::query::response(std::move(*expected_query),
                                                             [&response_buffer, &rejected_response_size](size_t size) noexcept -> std::span<std::byte> {
                                                               if (!details::has_memory_for_response_process(size)) [[unlikely]] {
                                                                 // An empty buffer makes `response` fail with TL_ERROR_RESULT_TOO_LARGE before anything is copied from the platform
                                                                 rejected_response_size = size;
                                                                 return {};
                                                               }

                                                               response_buffer.resize(size);
                                                               return response_buffer;
                                                             })};
  if (rejected_response_size) [[unlikely]] {
    kphp::log::warning("subscribe response doesn't fit into the instance memory and was rejected: size -> {}", *rejected_response_size);
    co_return std::unexpected{kphp::confdata::subscribe_error::not_enough_memory};
  }

  if (!expected_response) [[unlikely]] {
    kphp::log::warning("failed to fetch subscribe response: {}", expected_response.error());
    co_return std::unexpected{kphp::confdata::subscribe_error::transport};
  }

  tl::fetcher tlf{*expected_response};
  tl::confdata::SubscribeResponse response{};
  if (!response.fetch(tlf)) [[unlikely]] {
    kphp::log::warning("failed to parse subscribe response");
    co_return std::unexpected{kphp::confdata::subscribe_error::malformed_response};
  }

  auto handled{
      std::visit(overloaded{
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
                 response.value)};
  if (handled && retained_response.has_value()) {
    retained_response->get() = std::move(response_buffer);
  }
  co_return std::move(handled);
}

// Fetches one consistent snapshot while retaining the encoded response pages.
// The handler can collect metadata from the initial parse; replay()
// reparses the same bytes later without another network synchronization.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto sync(std::string_view confdata_proxy_actor,
          event_handler_type event_handler) noexcept -> kphp::coro::task<std::expected<snapshot, kphp::confdata::subscribe_error>> {
  snapshot snapshot{};
  for (; !snapshot.m_pagination.m_has_synced;) {
    snapshot.m_pages.emplace_back();
    if (auto expected{co_await subscribe(confdata_proxy_actor, snapshot.m_pagination, event_handler, snapshot.m_pages.back())}; !expected) [[unlikely]] {
      co_return std::unexpected{expected.error()};
    }
  }
  co_return std::move(snapshot);
}

// Takes ownership of previously fetched snapshot pages and releases each page after
// its parsed event descriptors have been consumed. On failure, remaining pages are released too.
template<std::predicate<std::span<const tl::confdata::KeyValuePair>> event_handler_type>
auto replay(encoded_snapshot_pages pages, const event_handler_type& event_handler) noexcept -> std::expected<void, kphp::confdata::subscribe_error> {
  for (auto& encoded_page : pages) {
    {
      tl::fetcher tlf{std::span<const std::byte>{encoded_page.data(), encoded_page.size()}};
      tl::confdata::SubscribeResponse response{};
      if (!response.fetch(tlf)) [[unlikely]] {
        return std::unexpected{kphp::confdata::subscribe_error::malformed_response};
      }

      if (std::holds_alternative<tl::confdata::subscribeResponseOldOffsetError>(response.value)) {
        return std::unexpected{kphp::confdata::subscribe_error::old_offset};
      }

      const auto& response_ok{std::get<tl::confdata::subscribeResponseOk>(response.value)};
      if (const auto& events{response_ok.events};
          events.size() != 0 && !std::invoke(event_handler, std::span<const tl::confdata::KeyValuePair>{events.value})) {
        return std::unexpected{kphp::confdata::subscribe_error::batch_rejected};
      }
    }
    // Make the processed page's memory reusable before parsing the next page.
    encoded_snapshot_page{}.swap(encoded_page);
  }
  return {};
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
    if (auto expected{co_await subscribe(confdata_proxy_actor, from, event_handler)}; !expected) [[unlikely]] {
      co_return std::unexpected{expected.error()};
    }
    co_await kphp::coro::io_scheduler::get().schedule(UPDATE_INTERVAL);
  }
}

} // namespace kphp::confdata
