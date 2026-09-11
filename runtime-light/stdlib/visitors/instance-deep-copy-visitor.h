// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <span>

#include "runtime-common/core/allocator/pool-allocator.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/visitors/instance-deep-basic-visitor.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::visitors {

// deep-copies an instance graph into a caller-provided memory block (e.g. shared memory), rewriting the original's fields to point at the copies.
// Copies are pinned with memory_ref_cnt (e.g. ExtraRefCnt::for_instance_cache) and never freed individually.
class instance_deep_copy_visitor final : kphp::visitors::instance_deep_basic_visitor<instance_deep_copy_visitor> {
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

  explicit instance_deep_copy_visitor(std::span<std::byte> memory_pool_buffer, ExtraRefCnt memory_ref_cnt) noexcept
      : Basic{*this, memory_ref_cnt},
        allocator{kphp::memory::pool_allocator::external_memory{}, memory_pool_buffer.data(), memory_pool_buffer.size(), /*oom_handling_mem_size=*/0} {}

  template<class T>
  bool process(array<T>& arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    RuntimeAllocator::get().with_allocator(allocator, [&arr]() noexcept { arr.mutate_if_shared(); });

    // copying an empty array yields the global empty-array singleton instead of a real copy -- nothing left to deep-copy
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      kphp::log::assertion(arr.begin_no_mutate() == arr.end_no_mutate());
      return true;
    }

    kphp::log::assertion(arr.get_reference_counter() == 1);
    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      arr.set_reference_counter_to(extra_ref_cnt);
    }
    // values of a primitive array were already memcpy'd by the forced copy above, and there are no string keys to copy
    const bool primitive_array{Basic::template is_primitive<T> && arr.has_no_string_keys()};
    return primitive_array || Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
  }

  bool process(string& str) noexcept {
    if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    RuntimeAllocator::get().with_allocator(allocator, [&str]() noexcept { str.make_not_shared(); });

    // make_not_shared may turn str back into a constant (e.g. empty or single-char strings are cached globally) -- check again
    if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    kphp::log::assertion(str.get_reference_counter() == 1);
    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      str.set_reference_counter_to(extra_ref_cnt);
    }
    return true;
  }

  bool process(mixed& value) noexcept {
    if (value.is_object()) {
      kphp::log::warning("cannot deep-copy a mixed value holding an object of class {}: objects inside mixed are not supported",
                         value.as_object()->get_class());
      return false;
    }
    return Basic::process(value);
  }

  template<class I>
  bool process_instance(class_instance<I>& instance) noexcept {
    // keep the original instance alive for the whole traversal: copied_instances_table uses raw pointers to originals as keys
    class_instance<I> instance_keepalive{instance};
    const bool result{process(instance)};
    this->copied_instances_table.clear();
    return result;
  }

private:
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

    RuntimeAllocator::get().with_allocator(allocator, [&instance]() noexcept { instance = instance.virtual_builtin_clone(); });
    copied_instance_ptr = instance.get_base_raw_ptr();

    if (const auto extra_ref_cnt{get_memory_ref_cnt()}; extra_ref_cnt != 0) {
      instance.set_reference_counter_to(extra_ref_cnt);
    }
    return Basic::process(instance);
  }

  kphp::memory::pool_allocator allocator;
  kphp::stl::unordered_map<void*, void*, kphp::memory::script_allocator> copied_instances_table;
};

} // namespace kphp::visitors
