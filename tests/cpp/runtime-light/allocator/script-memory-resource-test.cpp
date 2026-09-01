// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-common/stdlib/serialization/serialize-functions.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/confdata-storage.h"

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

auto start_update(kphp::confdata::storage& storage) noexcept -> kphp::confdata::storage::editor {
  auto editor{storage.start_update()};
  CHECK(editor.has_value());
  return std::move(*editor);
}

template<std::invocable value_factory_type>
requires std::same_as<std::invoke_result_t<value_factory_type>, mixed> && std::is_nothrow_invocable_v<value_factory_type>
auto upsert_shared(kphp::confdata::storage::editor& editor, std::string_view key, value_factory_type&& value_factory) noexcept -> bool {
  return editor.upsert(key, std::forward<value_factory_type>(value_factory));
}

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

  kphp::memory::with_script_memory_resource(shared_resource, [&] noexcept {
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

  kphp::memory::with_script_memory_resource(outer_resource, [&] noexcept {
    auto* outer_memory_before{script_allocator.allocate(32)};
    CHECK(contains(outer_storage, outer_memory_before));

    kphp::memory::with_script_memory_resource(inner_resource, [&] noexcept {
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
  kphp::memory::with_script_memory_resource(shared_resource, [&] noexcept {
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

auto test_confdata_storage_keeps_pinned_samples_alive() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  const auto logical_storage{std::span{shared_storage}.first(shared_storage.size() - 13)};
  kphp::confdata::storage storage{};
  CHECK(storage.init(logical_storage).has_value());
  CHECK(storage.memory().data() == shared_storage.data());
  CHECK(storage.memory().size() == logical_storage.size());
  const auto default_memory_before{RuntimeAllocator::get().memory_resource.get_memory_stats().memory_used};

  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "persistent-key", [] noexcept { return mixed{string{"persistent-value"}}; }));
  initial_update.commit();

  kphp::confdata::storage reader{};
  CHECK(reader.open(shared_storage).has_value());
  CHECK(reader.memory().size() == logical_storage.size());
  const auto old_sample{storage.acquire_active_sample()};
  CHECK(reader.values(old_sample).size() == 1);
  const auto& [key, value]{*reader.values(old_sample).begin()};
  CHECK(contains(shared_storage, std::addressof(*reader.values(old_sample).begin())));
  CHECK(contains(shared_storage, key.c_str()));
  CHECK(value.is_string());
  CHECK(contains(shared_storage, value.as_string().c_str()));
  CHECK(RuntimeAllocator::get().memory_resource.get_memory_stats().memory_used == default_memory_before);

  auto deletion{start_update(storage)};
  CHECK(deletion.erase("persistent-key"));
  deletion.commit();

  const auto new_sample{storage.acquire_active_sample()};
  CHECK(reader.values(new_sample).empty());
  CHECK(reader.values(old_sample).size() == 1);
  storage.release_sample(new_sample);
  storage.release_sample(old_sample);
  reader.close();
  storage.close();
  CHECK(!storage.is_initialized());
  CHECK(storage.memory().empty());
}

auto test_confdata_storage_backpressures_when_every_sample_is_pinned() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());

  std::vector<kphp::confdata::storage::sample_id> pinned_samples;
  auto sync_editor{storage.start_sync()};
  sync_editor.commit();
  pinned_samples.emplace_back(storage.acquire_active_sample());
  while (auto update{storage.start_update()}) {
    update->commit();
    pinned_samples.emplace_back(storage.acquire_active_sample());
  }
  CHECK(!pinned_samples.empty());
  CHECK(!storage.start_update().has_value());

  for (const auto sample : pinned_samples) {
    storage.release_sample(sample);
  }
  auto update{start_update(storage)};
  update.cancel();
  storage.close();
}

auto test_confdata_storage_deletion_retires_garbage_with_the_old_sample() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());

  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "a.b.c", [] noexcept { return mixed{string{"value"}}; }));
  CHECK(upsert_shared(initial_update, "plain", [] noexcept { return mixed{string{"plain-value"}}; }));
  initial_update.commit();

  const auto old_sample{storage.acquire_active_sample()};
  CHECK(storage.values(old_sample).size() == 3);

  auto deletion{start_update(storage)};
  CHECK(deletion.erase("a.b.c"));
  CHECK(deletion.erase("plain"));
  deletion.commit();

  const auto new_sample{storage.acquire_active_sample()};
  CHECK(storage.values(new_sample).empty());
  CHECK(storage.values(old_sample).size() == 3);
  storage.release_sample(new_sample);
  storage.release_sample(old_sample);

  // Starting another update reclaims the now-unpinned retired sample and its
  // shallow/deep garbage before choosing an update target.
  auto reclaim{start_update(storage)};
  reclaim.cancel();
  storage.close();
}

auto test_confdata_storage_reclaims_retired_samples_in_generation_order() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());

  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "key", [] noexcept { return mixed{string{"value"}}; }));
  initial_update.commit();
  const auto oldest_sample{storage.acquire_active_sample()};

  auto copied_update{start_update(storage)};
  copied_update.commit();
  const auto newer_sample{storage.acquire_active_sample()};

  auto deletion{start_update(storage)};
  CHECK(deletion.erase("key"));
  deletion.commit();
  storage.release_sample(newer_sample);

  // The newer retired sample owns the deleted value's garbage, but it cannot
  // be reclaimed before an older reader that still references that value.
  auto blocked_reclamation{start_update(storage)};
  const auto old_value{storage.values(oldest_sample).find(string{"key"})};
  CHECK(old_value != storage.values(oldest_sample).end());
  CHECK(old_value->second.is_string());
  CHECK(old_value->second.as_string() == string{"value"});
  blocked_reclamation.cancel();

  storage.release_sample(oldest_sample);
  auto reclamation{start_update(storage)};
  reclamation.cancel();
  storage.close();
}

auto test_confdata_storage_preserves_global_constants() noexcept -> void {
  static constexpr std::string_view GLOBAL_VALUE{"global"};
  alignas(std::max_align_t) std::array<std::byte, string::inner_sizeof() + GLOBAL_VALUE.size() + 1> global_value_memory{};
  const string global_value{string::make_const_string_on_memory(GLOBAL_VALUE.data(), static_cast<string::size_type>(GLOBAL_VALUE.size()),
                                                                global_value_memory.data(), global_value_memory.size())};

  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());
  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "key", [&global_value] noexcept { return mixed{global_value}; }));
  initial_update.commit();

  const auto old_sample{storage.acquire_active_sample()};
  const auto value_it{storage.values(old_sample).find(string{"key"})};
  CHECK(value_it != storage.values(old_sample).end());
  CHECK(value_it->second.is_reference_counter(ExtraRefCnt::for_global_const));

  auto deletion{start_update(storage)};
  CHECK(deletion.erase("key"));
  deletion.commit();
  storage.release_sample(old_sample);
  auto reclamation{start_update(storage)};
  reclamation.cancel();
  storage.close();
}

auto test_confdata_storage_deletes_all_predefined_representations() noexcept -> void {
  static constexpr std::array<std::string_view, 2> PREDEFINED_WILDCARDS{"pre", "prefix"};

  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());
  CHECK(storage.initialize_wildcards(PREDEFINED_WILDCARDS).has_value());

  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "prefix.value", [] noexcept { return mixed{string{"value"}}; }));
  initial_update.commit();

  kphp::confdata::storage reader{};
  CHECK(reader.open(storage.memory()).has_value());
  CHECK(reader.wildcards().contains("pre"));
  CHECK(reader.wildcards().contains("prefix"));
  const auto old_sample{storage.acquire_active_sample()};
  CHECK(reader.values(old_sample).size() == 3);
  const auto value_in = [&reader, old_sample](std::string_view section, std::string_view remainder) noexcept -> const mixed* {
    const string section_key{section.data(), static_cast<string::size_type>(section.size())};
    const auto section_it{reader.values(old_sample).find(section_key)};
    CHECK(section_it != reader.values(old_sample).end());
    CHECK(section_it->second.is_array());
    const string remainder_key{remainder.data(), static_cast<string::size_type>(remainder.size())};
    return section_it->second.as_array().find_value(remainder_key);
  };
  const auto* shortest_value{value_in("pre", "fix.value")};
  const auto* longest_value{value_in("prefix", ".value")};
  const auto* implicit_value{value_in("prefix.", "value")};
  CHECK(shortest_value != nullptr && longest_value != nullptr && implicit_value != nullptr);
  CHECK(shortest_value->is_string() && shortest_value->as_string() == string{"value"});
  CHECK(shortest_value->as_string().c_str() == longest_value->as_string().c_str());
  CHECK(shortest_value->as_string().c_str() == implicit_value->as_string().c_str());

  auto deletion{start_update(storage)};
  CHECK(deletion.erase("prefix.value"));
  deletion.commit();

  const auto new_sample{storage.acquire_active_sample()};
  CHECK(reader.values(new_sample).empty());
  CHECK(reader.values(old_sample).size() == 3);
  storage.release_sample(new_sample);
  storage.release_sample(old_sample);
  reader.close();

  auto reclamation{start_update(storage)};
  reclamation.cancel();
  storage.close();
}

auto test_confdata_storage_update_is_atomic() noexcept -> void {
  static constexpr std::array<std::string_view, 2> PREDEFINED_WILDCARDS{"pre", "prefix"};
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());
  CHECK(storage.initialize_wildcards(PREDEFINED_WILDCARDS).has_value());

  auto initial_update{storage.start_sync()};
  CHECK(upsert_shared(initial_update, "old-key", [] noexcept { return mixed{string{"old-value"}}; }));
  CHECK(upsert_shared(initial_update, "prefix.value", [] noexcept -> mixed {
    array<mixed> decoded{};
    decoded.set_value(string{"nested"}, mixed{string{"shared-old-value"}});
    return mixed{std::move(decoded)};
  }));
  initial_update.commit();

  const auto old_sample{storage.acquire_active_sample()};
  CHECK(storage.values(old_sample).contains(string{"old-key"}));

  {
    auto rolled_back{start_update(storage)};
    CHECK(rolled_back.erase("old-key"));
    CHECK(rolled_back.erase("prefix.value"));
    CHECK(upsert_shared(rolled_back, "new-key", [] noexcept -> mixed {
      array<mixed> decoded{};
      decoded.set_value(string{"field"}, mixed{42});
      return mixed{std::move(decoded)};
    }));

    const auto visible_sample{storage.acquire_active_sample()};
    CHECK(visible_sample == old_sample);
    CHECK(storage.values(visible_sample).contains(string{"old-key"}));
    CHECK(!storage.values(visible_sample).contains(string{"new-key"}));
    storage.release_sample(visible_sample);
    // The editor destructor rolls this unpublished update back.
  }

  const auto after_rollback{storage.acquire_active_sample()};
  CHECK(after_rollback == old_sample);
  storage.release_sample(after_rollback);

  auto update{start_update(storage)};
  CHECK(update.erase("old-key"));
  CHECK(update.erase("prefix.value"));
  CHECK(upsert_shared(update, "new-key", [] noexcept -> mixed {
    array<mixed> decoded{};
    decoded.set_value(string{"field"}, mixed{42});
    return mixed{std::move(decoded)};
  }));
  CHECK(storage.values(old_sample).contains(string{"old-key"}));
  update.commit();

  const auto new_sample{storage.acquire_active_sample()};
  CHECK(!storage.values(new_sample).contains(string{"old-key"}));
  const auto new_value{storage.values(new_sample).find(string{"new-key"})};
  CHECK(new_value != storage.values(new_sample).end());
  CHECK(new_value->second.is_array());
  const auto* field{new_value->second.as_array().find_value(string{"field"})};
  CHECK(field != nullptr && field->is_int() && field->as_int() == 42);
  CHECK(storage.values(old_sample).contains(string{"old-key"}));

  storage.release_sample(new_sample);
  storage.release_sample(old_sample);
  auto reclamation{start_update(storage)};
  reclamation.cancel();
  storage.close();
}

auto test_confdata_storage_decoded_values() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> shared_storage{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(shared_storage).has_value());

  auto update{storage.start_sync()};
  CHECK(upsert_shared(update, "plain", [] noexcept { return mixed{string{"plain-value"}}; }));
  CHECK(upsert_shared(update, "json", [] noexcept -> mixed {
    const auto json{json_decode(R"({"name":"k2","count":2,"nested":[true,null]})")};
    CHECK(json.has_value());
    return *json;
  }));
  CHECK(upsert_shared(update, "php", [] noexcept -> mixed {
    static constexpr std::string_view PHP_SERIALIZED{R"(a:2:{s:4:"name";s:2:"k2";i:5;s:5:"value";})"};
    const mixed php{unserialize_raw(PHP_SERIALIZED.data(), static_cast<int32_t>(PHP_SERIALIZED.size()))};
    CHECK(php.is_array());
    return php;
  }));
  update.commit();

  const auto sample{storage.acquire_active_sample()};
  const auto& values{storage.values(sample)};

  const auto plain{values.find(string{"plain"})};
  CHECK(plain != values.end());
  CHECK(plain->second.is_string() && plain->second.as_string() == string{"plain-value"});
  CHECK(plain->second.is_reference_counter(ExtraRefCnt::for_confdata));

  const auto json{values.find(string{"json"})};
  CHECK(json != values.end() && json->second.is_array());
  const auto* json_name{json->second.as_array().find_value(string{"name"})};
  const auto* json_count{json->second.as_array().find_value(string{"count"})};
  CHECK(json_name != nullptr && json_name->is_string() && json_name->as_string() == string{"k2"});
  CHECK(json_count != nullptr && json_count->is_int() && json_count->as_int() == 2);
  CHECK(json->second.is_reference_counter(ExtraRefCnt::for_confdata));
  CHECK(json_name->is_reference_counter(ExtraRefCnt::for_confdata));

  const auto php{values.find(string{"php"})};
  CHECK(php != values.end() && php->second.is_array());
  const auto* php_name{php->second.as_array().find_value(string{"name"})};
  const auto* php_value{php->second.as_array().find_value(mixed{5})};
  CHECK(php_name != nullptr && php_name->is_string() && php_name->as_string() == string{"k2"});
  CHECK(php_value != nullptr && php_value->is_string() && php_value->as_string() == string{"value"});

  storage.release_sample(sample);
  storage.close();
}

auto test_confdata_storage_rejects_invalid_memory() noexcept -> void {
  alignas(std::max_align_t) std::array<std::byte, TEST_RESOURCE_SIZE> memory{};

  kphp::confdata::storage misaligned_storage{};
  const auto misaligned{misaligned_storage.init(std::span{memory}.subspan(1))};
  CHECK(!misaligned.has_value());
  CHECK(misaligned.error() == kphp::confdata::storage_error::misaligned_buffer);

  kphp::confdata::storage undersized_storage{};
  const auto undersized{undersized_storage.init(std::span{memory}.first(1))};
  CHECK(!undersized.has_value());
  CHECK(undersized.error() == kphp::confdata::storage_error::insufficient_buffer);

  const auto overflow{kphp::confdata::storage::memory_size(std::numeric_limits<size_t>::max())};
  CHECK(!overflow.has_value());
  CHECK(overflow.error() == kphp::confdata::storage_error::size_overflow);

  kphp::confdata::storage invalid_storage{};
  const auto invalid{invalid_storage.open(memory)};
  CHECK(!invalid.has_value());
  CHECK(invalid.error() == kphp::confdata::storage_error::invalid_storage);
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

void php_warning(const char* /*unused*/, ...) {}

void php_error(const char* /*unused*/, ...) {}

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
  test_confdata_storage_keeps_pinned_samples_alive();
  test_confdata_storage_backpressures_when_every_sample_is_pinned();
  test_confdata_storage_deletion_retires_garbage_with_the_old_sample();
  test_confdata_storage_reclaims_retired_samples_in_generation_order();
  test_confdata_storage_preserves_global_constants();
  test_confdata_storage_deletes_all_predefined_representations();
  test_confdata_storage_update_is_atomic();
  test_confdata_storage_decoded_values();
  test_confdata_storage_rejects_invalid_memory();
  RuntimeAllocator::get().free();
  return 0;
}
