// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/instance-state.h"

#include <chrono>
#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/components/confdata/confdata-proxy/sync-functions.h"
#include "runtime-light/components/confdata/confdata-proxy/tl.h"
#include "runtime-light/components/confdata/state/component-state.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/when-all.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"

namespace {

auto sync_handler(std::span<const tl::confdata::KeyValuePair> events) noexcept -> void {
  kphp::log::info("got {} events on sync", events.size());
}

auto update_handler(std::span<const tl::confdata::KeyValuePair> events) noexcept -> void {
  kphp::log::info("got {} events on update", events.size());
}

} // namespace

auto InstanceState::init() noexcept -> void {
  const auto shared_memory_size{kphp::confdata::storage::memory_size(ComponentState::get().m_confdata_memory_limit)};
  if (!shared_memory_size) [[unlikely]] {
    kphp::log::error("invalid confdata shared memory size: error -> {}", std::to_underlying(shared_memory_size.error()));
  }
  auto shared_memory{k2::alloc_shared_memory(*shared_memory_size, kphp::confdata::storage::memory_alignment())};
  if (!shared_memory) [[unlikely]] {
    kphp::log::error("failed to allocate confdata shared memory: error -> {}", shared_memory.error());
  }
  auto initialized_storage{m_confdata_storage.init({static_cast<std::byte*>(*shared_memory), *shared_memory_size})};
  if (!initialized_storage) [[unlikely]] {
    kphp::log::error("failed to initialize confdata shared memory: error -> {}", std::to_underlying(initialized_storage.error()));
  }

  auto main_task{run()};
  // initialize async stack
  auto& main_task_async_stack_frame{main_task.get_handle().promise().get_async_stack_frame()};
  main_task_async_stack_frame.async_stack_root = std::addressof(m_coroutine_instance_state.coroutine_stack_root);
  m_coroutine_instance_state.coroutine_stack_root.top_async_stack_frame = std::addressof(main_task_async_stack_frame);
  // spawn main task onto the scheduler
  kphp::log::assertion(m_io_scheduler.spawn(std::move(main_task)));
}

auto InstanceState::run() noexcept -> kphp::coro::task<> {
  co_await kphp::coro::when_all(service_loop(), accept_loop()); // both never return
  kphp::log::assertion(false);
}

auto InstanceState::accept_loop() noexcept -> kphp::coro::task<> {
  for (;;) {
    auto opt_stream{co_await kphp::component::stream::accept()};
    if (!opt_stream.has_value()) [[unlikely]] {
      kphp::log::warning("failed to accept a stream");
      continue;
    }
    auto request_stream{std::move(*opt_stream)};
    kphp::log::info("accepted a stream: descriptor -> {}", request_stream.descriptor());

    // dummy implementation: drain the request and close
    if (auto expected{co_await request_stream.read_all([](std::span<const std::byte>) noexcept {})}; !expected) [[unlikely]] {
      kphp::log::warning("failed to read a request: error -> {}", expected.error());
    }
  }
}

auto InstanceState::service_loop() noexcept -> kphp::coro::task<> {
  static constexpr auto CONFDATA_RETRY_INTERVAL{std::chrono::seconds{1}};
  const std::string_view confdata_proxy_actor{ComponentState::get().m_confdata_proxy_actor_name};

  for (;;) {
    if (!m_pagination.m_has_synced) {
      auto sync{co_await kphp::confdata::sync(confdata_proxy_actor, sync_handler)};
      if (!sync) [[unlikely]] {
        kphp::log::warning("confdata sync failed: error -> {}, retrying", std::to_underlying(std::move(sync).error()));
        co_await m_io_scheduler.schedule(CONFDATA_RETRY_INTERVAL);
        continue;
      }
      m_pagination = *std::move(sync);
      m_warmup_status = InstanceState::warmup_status::done;
    }

    auto update{co_await kphp::confdata::update(confdata_proxy_actor, m_pagination, update_handler)};
    // update returns only on error; m_pagination was advanced in place up to the last applied batch
    kphp::log::assertion(!update.has_value());
    switch (update.error()) {
    case kphp::confdata::subscribe_error::old_offset:
    case kphp::confdata::subscribe_error::not_synced:
      // local version is too old: clean re-sync required
      kphp::log::warning("confdata update failed: error -> {}, resyncing", std::to_underlying(std::move(update).error()));
      m_pagination = {};
      break;
    case kphp::confdata::subscribe_error::transport:
    case kphp::confdata::subscribe_error::malformed_response:
      // pagination is still valid; the longpoll resumes from the last applied position
      kphp::log::warning("confdata update failed: error -> {}, retrying", std::to_underlying(std::move(update).error()));
      break;
    }
    co_await m_io_scheduler.schedule(CONFDATA_RETRY_INTERVAL);
  }
}
