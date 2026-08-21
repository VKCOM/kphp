// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace {

constexpr auto DEFAULT_ALLOCATOR_SIZE{static_cast<size_t>(1024U * 1024U)};
constexpr auto TEST_RESOURCE_SIZE{static_cast<size_t>(64U * 1024U)};

auto check(bool condition, const char* expression, int line) noexcept -> void {
  if (!condition) [[unlikely]] {
    static_cast<void>(std::fprintf(stderr, "check failed at line %d: %s\n", line, expression));
    std::abort();
  }
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

template<size_t size>
auto contains(const std::array<std::byte, size>& storage, const void* memory) noexcept -> bool {
  const auto begin{reinterpret_cast<uintptr_t>(storage.data())};
  const auto end{begin + storage.size()};
  const auto address{reinterpret_cast<uintptr_t>(memory)};
  return begin <= address && address < end;
}

struct noexcept_void_callback final {
  auto operator()() const noexcept -> void {}
};

struct throwing_void_callback final {
  auto operator()() const -> void {}
};

struct noexcept_value_callback final {
  auto operator()() const noexcept -> int {
    return 0;
  }
};

template<typename callback_type>
concept valid_script_memory_callback = requires(memory_resource::unsynchronized_pool_resource& resource, callback_type callback) {
  kphp::memory::with_script_memory_resource(resource, std::move(callback));
};

static_assert(valid_script_memory_callback<noexcept_void_callback>);
static_assert(!valid_script_memory_callback<throwing_void_callback>);
static_assert(!valid_script_memory_callback<noexcept_value_callback>);

auto test_routes_script_allocations_and_restores_default() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  memory_resource::unsynchronized_pool_resource shared_resource{};
  shared_resource.init(shared_storage.data(), shared_storage.size());

  auto& allocator{RuntimeAllocator::get()};
  auto& default_resource{allocator.memory_resource};
  const auto default_memory_before{default_resource.get_memory_stats().memory_used};

  kphp::memory::with_script_memory_resource(shared_resource, [&]() noexcept {
    CHECK(std::addressof(allocator.current_script_memory_resource()) == std::addressof(shared_resource));

    kphp::memory::script_allocator<std::byte> script_allocator{};
    auto* memory{script_allocator.allocate(64)};
    CHECK(contains(shared_storage, memory));
    CHECK(shared_resource.get_memory_stats().memory_used != 0);
    CHECK(default_resource.get_memory_stats().memory_used == default_memory_before);
    script_allocator.deallocate(memory, 64);
  });

  CHECK(std::addressof(allocator.current_script_memory_resource()) == std::addressof(default_resource));
  CHECK(shared_resource.get_memory_stats().memory_used == 0);
  CHECK(default_resource.get_memory_stats().memory_used == default_memory_before);
}

auto test_nested_resources_restore_previous_target() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> outer_storage{};
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> inner_storage{};
  memory_resource::unsynchronized_pool_resource outer_resource{};
  memory_resource::unsynchronized_pool_resource inner_resource{};
  outer_resource.init(outer_storage.data(), outer_storage.size());
  inner_resource.init(inner_storage.data(), inner_storage.size());

  auto& allocator{RuntimeAllocator::get()};
  kphp::memory::script_allocator<std::byte> script_allocator{};

  kphp::memory::with_script_memory_resource(outer_resource, [&]() noexcept {
    auto* outer_memory_before{script_allocator.allocate(32)};
    CHECK(contains(outer_storage, outer_memory_before));

    kphp::memory::with_script_memory_resource(inner_resource, [&]() noexcept {
      auto* inner_memory{script_allocator.allocate(32)};
      CHECK(contains(inner_storage, inner_memory));
      script_allocator.deallocate(inner_memory, 32);
    });

    CHECK(std::addressof(allocator.current_script_memory_resource()) == std::addressof(outer_resource));
    auto* outer_memory_after{script_allocator.allocate(32)};
    CHECK(contains(outer_storage, outer_memory_after));
    script_allocator.deallocate(outer_memory_after, 32);
    script_allocator.deallocate(outer_memory_before, 32);
  });

  CHECK(std::addressof(allocator.current_script_memory_resource()) == std::addressof(allocator.memory_resource));
  CHECK(outer_resource.get_memory_stats().memory_used == 0);
  CHECK(inner_resource.get_memory_stats().memory_used == 0);
}

auto test_zeroing_and_reallocation_use_replacement_resource() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  memory_resource::unsynchronized_pool_resource shared_resource{};
  shared_resource.init(shared_storage.data(), shared_storage.size());

  auto& allocator{RuntimeAllocator::get()};
  kphp::memory::with_script_memory_resource(shared_resource, [&]() noexcept {
    constexpr auto initial_size{static_cast<size_t>(32)};
    constexpr auto expanded_size{static_cast<size_t>(128)};

    auto* zeroed{static_cast<std::byte*>(allocator.alloc0_script_memory(initial_size))};
    CHECK(contains(shared_storage, zeroed));
    CHECK(std::ranges::all_of(std::span{zeroed, initial_size}, [](std::byte value) noexcept { return value == std::byte{}; }));

    std::ranges::fill(std::span{zeroed, initial_size}, std::byte{0x5A});
    auto* expanded{static_cast<std::byte*>(allocator.realloc_script_memory(zeroed, expanded_size, initial_size))};
    CHECK(contains(shared_storage, expanded));
    CHECK(std::ranges::all_of(std::span{expanded, initial_size}, [](std::byte value) noexcept { return value == std::byte{0x5A}; }));
    allocator.free_script_memory(expanded, expanded_size);
  });

  CHECK(shared_resource.get_memory_stats().memory_used == 0);
}

auto test_default_resource_can_request_extra_memory() noexcept -> void {
  auto& allocator{RuntimeAllocator::get()};
  constexpr auto allocation_size{DEFAULT_ALLOCATOR_SIZE * 2};

  auto* memory{allocator.alloc_script_memory(allocation_size)};
  CHECK(memory != nullptr);
  CHECK(allocator.memory_resource.get_extra_memory_head()->get_pool_payload_size() != 0);
  allocator.free_script_memory(memory, allocation_size);
}

} // namespace

extern "C" void* k2_alloc(size_t size, size_t align) {
  const auto actual_align{std::max(align, alignof(std::max_align_t))};
  const auto actual_size{(size + actual_align - 1) / actual_align * actual_align};
  return std::aligned_alloc(actual_align, actual_size);
}

extern "C" void* k2_realloc(void* memory, size_t new_size) {
  return std::realloc(memory, new_size);
}

extern "C" void k2_free(void* memory) {
  std::free(memory);
}

extern "C" void k2_log(size_t /*level*/, size_t /*len*/, const char* /*msg*/, size_t /*kv_count*/, const LogKeyValuePair* /*kv_pairs*/) {}

extern "C" void k2_exit(int32_t /*exit_code*/) {
  std::abort();
}

void runtime_error(const char* /*unused*/, ...) {}

[[noreturn]] void critical_error_handler() {
  std::abort();
}

[[noreturn]] void php_assert__(const char* /*unused*/, const char* /*unused*/, int /*unused*/) {
  std::abort();
}

auto AllocatorState::get() noexcept -> const AllocatorState& {
  static AllocatorState allocator_state{DEFAULT_ALLOCATOR_SIZE, DEFAULT_ALLOCATOR_SIZE, 0};
  return allocator_state;
}

auto main() -> int {
  test_routes_script_allocations_and_restores_default();
  test_nested_resources_restore_previous_target();
  test_zeroing_and_reallocation_use_replacement_resource();
  test_default_resource_can_request_extra_memory();
  RuntimeAllocator::get().free();
  return 0;
}
