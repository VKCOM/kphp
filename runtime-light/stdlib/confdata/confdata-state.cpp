// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-state.h"

#include <memory>
#include <span>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/confdata-constants.h"
#include "runtime-light/stdlib/confdata/confdata-reader-lease.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto ConfdataInstanceState::init() noexcept -> kphp::coro::task<> {
  kphp::log::assertion(!is_initialized());
  kphp::log::assertion(!m_reader_lease.has_value());

  auto lease_stream{kphp::component::stream::open(kphp::confdata::COMPONENT_LINK_ALIAS, k2::stream_kind::component)};
  if (!lease_stream) {
    co_return;
  }

  kphp::confdata::reader_lease lease{};
  const auto read{co_await lease_stream->read(std::as_writable_bytes(std::span{std::addressof(lease), 1}))};
  if (!read || *read != sizeof(lease) || !lease.is_valid()) [[unlikely]] {
    co_return kphp::log::warning("failed to acquire a valid confdata reader lease");
  }

  const auto shared_memory{k2::get_shared_memory(lease.shared_memory_name())};
  if (!shared_memory) {
    co_return kphp::log::warning("failed to get confdata shared memory: error -> {}", shared_memory.error());
  }
  if (const auto opened{m_storage.open(*shared_memory)}; !opened) [[unlikely]] {
    co_return kphp::log::warning("failed to open confdata shared memory: error -> {}", std::to_underlying(opened.error()));
  }

  m_sample_id = lease.sample_id();
  m_reader_lease.emplace(std::move(*lease_stream));
}
