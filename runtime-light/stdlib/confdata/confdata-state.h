// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/confdata/confdata-storage.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"

class ConfdataInstanceState final : private vk::not_copyable {
  kphp::confdata::storage m_storage;
  std::optional<kphp::component::stream> m_reader_lease;
  kphp::confdata::storage::sample_id m_sample_id{kphp::confdata::storage::INVALID_SAMPLE_ID};

public:
  ConfdataInstanceState() noexcept = default;

  auto init() noexcept -> kphp::coro::task<>;
  auto release() noexcept -> void;
  auto is_initialized() const noexcept -> bool;
  auto values() const noexcept -> const kphp::confdata::storage::map_type&;
  auto wildcards() const noexcept -> const kphp::confdata::predefined_wildcards&;

  static auto get() noexcept -> ConfdataInstanceState&;
};

inline auto ConfdataInstanceState::release() noexcept -> void {
  if (!is_initialized()) {
    return;
  }
  m_storage.close();
  m_sample_id = kphp::confdata::storage::INVALID_SAMPLE_ID;
  // Closing the stream is the release signal; the component owns the reader
  // count and also observes this close when K2 terminates an instance abruptly.
  m_reader_lease.reset();
}

inline auto ConfdataInstanceState::is_initialized() const noexcept -> bool {
  return m_sample_id != kphp::confdata::storage::INVALID_SAMPLE_ID;
}

inline auto ConfdataInstanceState::values() const noexcept -> const kphp::confdata::storage::map_type& {
  kphp::log::assertion(is_initialized());
  return m_storage.values(m_sample_id);
}

inline auto ConfdataInstanceState::wildcards() const noexcept -> const kphp::confdata::predefined_wildcards& {
  kphp::log::assertion(is_initialized());
  return m_storage.wildcards();
}
