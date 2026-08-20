// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/memory-resource/details/memory_chunk_list.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/visitors/instance-deep-basic-visitor.h"

namespace kphp::visitors {

// computes how many bytes instance_deep_copy_visitor will carve for an instance graph,
// so that the destination memory block can be allocated upfront; the two visitors must stay in sync
class instance_deep_size_count_visitor final : kphp::visitors::instance_deep_basic_visitor<instance_deep_size_count_visitor> {
public:
  friend class kphp::visitors::instance_deep_basic_visitor<instance_deep_size_count_visitor>;

  using Basic = kphp::visitors::instance_deep_basic_visitor<instance_deep_size_count_visitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::get_memory_ref_cnt;

  instance_deep_size_count_visitor(const instance_deep_size_count_visitor&) = delete;
  instance_deep_size_count_visitor(instance_deep_size_count_visitor&&) = delete;
  instance_deep_size_count_visitor& operator=(const instance_deep_size_count_visitor&) = delete;
  instance_deep_size_count_visitor& operator=(instance_deep_size_count_visitor&&) = delete;
  ~instance_deep_size_count_visitor() = default;

  explicit instance_deep_size_count_visitor() noexcept
      : Basic{*this} {}

  template<class T>
  bool process(array<T>& arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    this->inner_size += memory_resource::details::align_for_chunk(arr.calculate_memory_for_copying());

    // primitive values are already accounted for wholesale above; only non-primitive values and string keys need traversal
    const bool primitive_array{is_primitive<T> && arr.has_no_string_keys()};
    return primitive_array || Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
  }

  bool process(string& str) noexcept {
    if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      this->inner_size += memory_resource::details::align_for_chunk(str.estimate_memory_usage());
    }

    return true;
  }

  template<class I>
  bool process_instance(class_instance<I>& instance) noexcept {
    const bool result{process(instance)};
    this->visited_instances_set.clear();
    return result;
  }

  size_t get_inner_size() const noexcept {
    return this->inner_size;
  }

private:
  template<class T>
  static constexpr bool is_primitive{vk::is_type_in_list<T, int64_t, double, bool, Optional<int64_t>, Optional<double>, Optional<bool>>::value};

  template<class I>
  bool process(class_instance<I>& instance) noexcept {
    if (!instance.is_null()) {
      void* instance_raw_ptr{instance.get()->get_instance_data_raw_ptr()};
      if (this->visited_instances_set.contains(instance_raw_ptr)) {
        return true;
      }
      this->inner_size += memory_resource::details::align_for_chunk(instance.estimate_memory_usage());
      this->visited_instances_set.emplace(instance_raw_ptr);
    }
    return Basic::process(instance);
  }

  size_t inner_size{0};
  kphp::stl::unordered_set<void*, kphp::memory::script_allocator> visited_instances_set;
};

} // namespace kphp::visitors
