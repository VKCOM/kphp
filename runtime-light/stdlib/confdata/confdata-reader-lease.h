// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include "runtime-light/stdlib/confdata/confdata-storage.h"

namespace kphp::confdata {

/** Fixed-size handshake sent when the component grants a reader sample lease. */
class reader_lease final {
  static constexpr uint64_t MAGIC{0x4b32'4344'4c45'4153}; // "K2CDLEAS"
  static constexpr uint32_t VERSION{1};
  static constexpr size_t MAX_SHARED_MEMORY_NAME_SIZE{128};

  /** Identifies this wire layout and rejects unrelated stream payloads. */
  uint64_t m_magic{MAGIC};
  /** Allows the handshake layout to evolve independently of storage layout. */
  uint32_t m_version{VERSION};
  /** Selects the immutable confdata generation pinned by the component. */
  storage::sample_id m_sample_id{storage::INVALID_SAMPLE_ID};
  /** Number of meaningful bytes in `m_shared_memory_name`. */
  uint32_t m_shared_memory_name_size{};
  /** Name passed to `k2_get_shared_memory`; it is not null-terminated. */
  std::array<char, MAX_SHARED_MEMORY_NAME_SIZE> m_shared_memory_name{};

public:
  reader_lease() noexcept = default;

  static auto create(std::string_view shared_memory_name, storage::sample_id sample_id) noexcept -> std::optional<reader_lease>;

  auto is_valid() const noexcept -> bool;
  auto sample_id() const noexcept -> storage::sample_id;
  auto shared_memory_name() const noexcept -> std::string_view;
};

inline auto reader_lease::create(std::string_view shared_memory_name, storage::sample_id sample_id) noexcept -> std::optional<reader_lease> {
  if (shared_memory_name.empty() || shared_memory_name.size() > MAX_SHARED_MEMORY_NAME_SIZE || shared_memory_name.contains('\0') ||
      !storage::is_valid_sample_id(sample_id)) [[unlikely]] {
    return std::nullopt;
  }

  reader_lease lease{};
  lease.m_sample_id = sample_id;
  lease.m_shared_memory_name_size = static_cast<uint32_t>(shared_memory_name.size());
  std::ranges::copy(shared_memory_name, lease.m_shared_memory_name.begin());
  return lease;
}

inline auto reader_lease::is_valid() const noexcept -> bool {
  return m_magic == MAGIC && m_version == VERSION && storage::is_valid_sample_id(m_sample_id) && m_shared_memory_name_size != 0 &&
         m_shared_memory_name_size <= m_shared_memory_name.size() && !shared_memory_name().contains('\0');
}

inline auto reader_lease::sample_id() const noexcept -> storage::sample_id {
  return m_sample_id;
}

inline auto reader_lease::shared_memory_name() const noexcept -> std::string_view {
  const auto size{std::min(static_cast<size_t>(m_shared_memory_name_size), m_shared_memory_name.size())};
  return {m_shared_memory_name.data(), size};
}

static_assert(std::is_trivially_copyable_v<reader_lease>);

} // namespace kphp::confdata
