// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/confdata-storage.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>

#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::confdata {

struct alignas(std::max_align_t) storage::shared_state final {
  memory_resource::unsynchronized_pool_resource resource{};
  alignas(map_type) std::byte map_storage[sizeof(map_type)]{};
};

auto storage::memory_size(size_t memory_limit) noexcept -> std::expected<size_t, storage_error> {
  static_assert(alignof(shared_state) == memory_alignment());
  if (memory_limit == 0) [[unlikely]] {
    return std::unexpected{storage_error::insufficient_buffer};
  }
  if (memory_limit > std::numeric_limits<size_t>::max() - sizeof(shared_state)) [[unlikely]] {
    return std::unexpected{storage_error::size_overflow};
  }
  return sizeof(shared_state) + memory_limit;
}

auto storage::init(std::span<std::byte> memory) noexcept -> std::expected<void, storage_error> {
  kphp::log::assertion(!is_initialized());
  if (reinterpret_cast<uintptr_t>(memory.data()) % alignof(shared_state) != 0) [[unlikely]] {
    return std::unexpected{storage_error::misaligned_buffer};
  }
  if (memory.size() <= sizeof(shared_state)) [[unlikely]] {
    return std::unexpected{storage_error::insufficient_buffer};
  }

  m_memory = memory;
  m_state = std::construct_at(reinterpret_cast<shared_state*>(m_memory.data()));
  auto pool_memory{memory.subspan(sizeof(shared_state))};
  m_state->resource.init(pool_memory.data(), pool_memory.size());
  kphp::memory::with_script_memory_resource(m_state->resource, [this] noexcept { std::construct_at(reinterpret_cast<map_type*>(m_state->map_storage)); });
  return {};
}

auto storage::destroy() noexcept -> void {
  kphp::log::assertion(is_initialized());
  kphp::memory::with_script_memory_resource(resource(), [this] noexcept { std::destroy_at(std::addressof(mutable_values())); });
  std::destroy_at(m_state);
  m_state = nullptr;
  m_memory = {};
}

auto storage::values() const noexcept -> const map_type& {
  kphp::log::assertion(is_initialized());
  return *std::launder(reinterpret_cast<const map_type*>(m_state->map_storage));
}

auto storage::mutable_values() noexcept -> map_type& {
  return const_cast<map_type&>(values());
}

auto storage::resource() noexcept -> memory_resource::unsynchronized_pool_resource& {
  kphp::log::assertion(is_initialized());
  return m_state->resource;
}

} // namespace kphp::confdata
