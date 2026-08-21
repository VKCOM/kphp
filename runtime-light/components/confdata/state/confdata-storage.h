// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator.h"

namespace kphp::confdata {

enum class storage_error : uint8_t { misaligned_buffer, insufficient_buffer, size_overflow };

class storage final : private vk::not_copyable {
public:
  using map_type = kphp::stl::map<string, mixed, kphp::memory::script_allocator, stl_string_less>;

  static constexpr auto memory_alignment() noexcept -> size_t {
    return alignof(std::max_align_t);
  }

  static auto memory_size(size_t memory_limit) noexcept -> std::expected<size_t, storage_error>;

  auto init(std::span<std::byte> memory) noexcept -> std::expected<void, storage_error>;
  auto destroy() noexcept -> void;

  auto is_initialized() const noexcept -> bool {
    return m_state != nullptr;
  }

  auto memory() const noexcept -> std::span<const std::byte> {
    return m_memory;
  }

  auto values() const noexcept -> const map_type&;

  template<std::invocable<map_type&> callback_type>
  requires std::same_as<std::invoke_result_t<callback_type, map_type&>, void> && std::is_nothrow_invocable_v<callback_type, map_type&>
  auto mutate(callback_type&& callback) noexcept -> void {
    kphp::memory::with_script_memory_resource(resource(), [&callback, this] noexcept { std::invoke(std::forward<callback_type>(callback), mutable_values()); });
  }

private:
  struct shared_state;

  auto mutable_values() noexcept -> map_type&;
  auto resource() noexcept -> memory_resource::unsynchronized_pool_resource&;

  shared_state* m_state{};
  std::span<std::byte> m_memory;
};

} // namespace kphp::confdata
