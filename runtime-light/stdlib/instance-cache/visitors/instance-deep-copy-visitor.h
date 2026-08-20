// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <span>

#include "common/containers/final_action.h"
#include "runtime-common/core/memory-resource/details/memory_chunk_list.h"
#include "runtime-common/core/memory-resource/monotonic_buffer_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/visitors/instance-deep-basic-visitor.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::visitors {

// deep-copies an instance graph into a caller-provided contiguous memory block (e.g. a shared memory region):
// every reachable array/string/instance body is recreated inside the block via memory_pool, and the original's
// fields are rewritten to point at the copies; all copies are pinned with memory_ref_cnt (e.g. ExtraRefCnt::for_instance_cache),
// so they are never freed individually — the owner of the block controls their lifetime
class instance_deep_copy_visitor : kphp::visitors::instance_deep_basic_visitor<instance_deep_copy_visitor> {
public:
  friend class kphp::visitors::instance_deep_basic_visitor<instance_deep_copy_visitor>;

  using Basic = kphp::visitors::instance_deep_basic_visitor<instance_deep_copy_visitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::get_memory_ref_cnt;

  instance_deep_copy_visitor(const instance_deep_copy_visitor&) = delete;
  instance_deep_copy_visitor(instance_deep_copy_visitor&&) = delete;
  instance_deep_copy_visitor& operator=(const instance_deep_copy_visitor&) = delete;
  instance_deep_copy_visitor& operator=(instance_deep_copy_visitor&&) = delete;
  ~instance_deep_copy_visitor() = default;

  explicit instance_deep_copy_visitor(std::span<std::byte> memory_pool_buffer, ExtraRefCnt memory_ref_cnt = ExtraRefCnt::extra_ref_cnt_value(0)) noexcept
      : Basic{*this, memory_ref_cnt} {
    this->memory_pool.init(memory_pool_buffer.data(), memory_pool_buffer.size());
  }

  template<class T>
  bool process(array<T>& arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    array<T> copied_array{carve(arr.calculate_memory_for_copying()), arr};
    const auto commit_copy{vk::finally([&arr, &copied_array]() noexcept { arr = std::move(copied_array); })};

    // copying an empty array yields the global empty-array singleton instead of a real copy — nothing left to deep-copy
    if (copied_array.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    kphp::log::assertion(copied_array.get_reference_counter() == 1);
    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      copied_array.set_reference_counter_to(extra_ref_cnt);
    }
    // values of a primitive array were already memcpy'd by the array copy constructor, and there are no string keys to copy
    const bool primitive_array{is_primitive<T> && copied_array.has_no_string_keys()};
    return primitive_array || Basic::process_range(copied_array.begin(), copied_array.end());
  }

  bool process(string& str) noexcept {
    if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    string copied_string{carve(str.estimate_memory_usage()), str};
    const auto commit_copy{vk::finally([&str, &copied_string]() noexcept { str = std::move(copied_string); })};

    kphp::log::assertion(copied_string.get_reference_counter() == 1);
    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      copied_string.set_reference_counter_to(extra_ref_cnt);
    }
    return true;
  }

  template<class I>
  bool process_instance(class_instance<I>& instance) noexcept {
    // keep the original instance alive for the whole traversal: copied_instances_table uses raw pointers to originals as keys
    class_instance<I> instance_copy{instance};
    const bool result{process(instance)};
    this->copied_instances_table.clear();
    return result;
  }

private:
  template<class T>
  static constexpr bool is_primitive{vk::is_type_in_list<T, int64_t, double, bool, Optional<int64_t>, Optional<double>, Optional<bool>>::value};

  template<class I>
  bool process(class_instance<I>& instance) noexcept {
    if (instance.is_null()) {
      return true;
    }

    auto& copied_instance_ptr{copied_instances_table[instance.get()->get_instance_data_raw_ptr()]};

    // shared or cyclic references resolve to the same copy, which is created on first visit
    if (copied_instance_ptr != nullptr) {
      instance = class_instance<I>::create_from_base_raw_ptr(copied_instance_ptr);
      return true;
    }

    instance = instance.virtual_builtin_clone(carve(instance.estimate_memory_usage()));
    copied_instance_ptr = instance.get_base_raw_ptr();

    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      instance.set_reference_counter_to(extra_ref_cnt);
    }
    return Basic::process(instance);
  }

  vk::span<std::byte> carve(size_t size) noexcept {
    const size_t aligned_size{memory_resource::details::align_for_chunk(size)};
    return {static_cast<std::byte*>(this->memory_pool.get_from_pool(aligned_size)), aligned_size};
  }

  memory_resource::monotonic_buffer_resource memory_pool;
  kphp::stl::unordered_map<void*, void*, kphp::memory::script_allocator> copied_instances_table;
};

} // namespace kphp::visitors
