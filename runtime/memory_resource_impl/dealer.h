// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime/memory_resource_impl/heap_resource.h"

namespace memory_resource {

class Dealer {
public:
  Dealer() noexcept;

  void set_script_resource_replacer(heap_resource& heap_replacer) noexcept {
    php_assert(!heap_replacer_);
    heap_replacer_ = &heap_replacer;
  }

  void set_current_script_resource(unsynchronized_pool_resource& current_script_resource) noexcept {
    current_script_resource_ = &current_script_resource;
  }

  void restore_default_script_resource() noexcept {
    set_current_script_resource(default_script_resource_);
  }

  bool is_default_allocator_used() const noexcept {
    return &default_script_resource_ == current_script_resource_;
  }

  void drop_replacer() noexcept {
    php_assert(heap_replacer_);
    heap_replacer_ = nullptr;
  }

  heap_resource* heap_script_resource_replacer() const noexcept {
    return heap_replacer_;
  }

  heap_resource& get_heap_resource() noexcept {
    return heap_resource_;
  }

  unsynchronized_pool_resource& current_script_resource() noexcept {
    return *current_script_resource_;
  }

private:
  heap_resource heap_resource_;
  unsynchronized_pool_resource default_script_resource_;

  unsynchronized_pool_resource* current_script_resource_{nullptr};
  memory_resource::heap_resource* heap_replacer_{nullptr};
};

} // namespace memory_resource
