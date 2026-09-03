// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/memory-resource/details/memory_chunk_list.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/visitors/instance-deep-basic-visitor.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::visitors {

// computes how many bytes instance_deep_copy_visitor will carve for an instance graph,
// so that the destination memory block can be allocated upfront.
// The two visitors must stay in sync.
class instance_deep_estimate_size_visitor final : kphp::visitors::instance_deep_basic_visitor<instance_deep_estimate_size_visitor> {
public:
  friend class kphp::visitors::instance_deep_basic_visitor<instance_deep_estimate_size_visitor>;

  using Basic = kphp::visitors::instance_deep_basic_visitor<instance_deep_estimate_size_visitor>;
  using Basic::process;
  using Basic::operator();

  instance_deep_estimate_size_visitor(const instance_deep_estimate_size_visitor&) = delete;
  instance_deep_estimate_size_visitor(instance_deep_estimate_size_visitor&&) = delete;
  instance_deep_estimate_size_visitor& operator=(const instance_deep_estimate_size_visitor&) = delete;
  instance_deep_estimate_size_visitor& operator=(instance_deep_estimate_size_visitor&&) = delete;
  ~instance_deep_estimate_size_visitor() = default;

  explicit instance_deep_estimate_size_visitor() noexcept
      : Basic{*this} {}

  template<class T>
  bool process(array<T>& arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }

    this->estimated_size += memory_resource::details::align_for_chunk(arr.calculate_memory_for_copying(), array<T>::alignment());

    // primitive values are already accounted for wholesale above.
    // Only non-primitive values and string keys need traversal.
    const bool primitive_array{Basic::template is_primitive<T> && arr.has_no_string_keys()};
    return primitive_array || Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
  }

  bool process(string& str) noexcept {
    if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      this->estimated_size += memory_resource::details::align_for_chunk(str.estimate_memory_usage(), string::alignment());
    }

    return true;
  }

  bool process(mixed& value) noexcept {
    if (value.is_object()) {
      kphp::log::warning("cannot estimate the size of a mixed value holding an object of class {}: objects inside mixed are not supported",
                         value.as_object()->get_class());
      return false;
    }
    return Basic::process(value);
  }

  template<class I>
  bool process_instance(class_instance<I>& instance) noexcept {
    const bool result{process(instance)};
    this->visited_instances_set.clear();
    return result;
  }

  size_t get_estimated_size() const noexcept {
    return this->estimated_size;
  }

private:
  template<class I>
  bool process(class_instance<I>& instance) noexcept {
    if (!instance.is_null()) {
      void* instance_raw_ptr{instance.get()->get_instance_data_raw_ptr()};
      if (this->visited_instances_set.contains(instance_raw_ptr)) {
        return true;
      }
      this->estimated_size += memory_resource::details::align_for_chunk(instance.estimate_memory_usage(), instance.alignment());
      this->visited_instances_set.emplace(instance_raw_ptr);
    }
    return Basic::process(instance);
  }

  size_t estimated_size{0};
  kphp::stl::unordered_set<void*, kphp::memory::script_allocator> visited_instances_set;
};

} // namespace kphp::visitors
