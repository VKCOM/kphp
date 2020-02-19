#pragma once
#include "runtime/memory_resource/heap_resource.h"
#include "runtime/memory_resource/synchronized_pool_resource.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

namespace memory_resource {

class Dealer {
public:
  Dealer() noexcept {
    set_script_resource_replacer(&heap_resource_);
  }

  void set_script_resource_replacer(memory_resource::synchronized_pool_resource *synchronized_replacer) noexcept {
    php_assert(!synchronized_replacer_);
    php_assert(!heap_replacer_);
    synchronized_replacer_ = synchronized_replacer;
  }

  void set_script_resource_replacer(memory_resource::heap_resource *heap_replacer) noexcept {
    php_assert(!synchronized_replacer_);
    php_assert(!heap_replacer_);
    heap_replacer_ = heap_replacer;
  }

  void drop_replacer() noexcept {
    php_assert(heap_replacer_ || synchronized_replacer_);
    synchronized_replacer_ = nullptr;
    heap_replacer_ = nullptr;
  }

  memory_resource::synchronized_pool_resource *synchronized_script_resource_replacer() const noexcept {
    return synchronized_replacer_;
  }

  memory_resource::heap_resource *heap_script_resource_replacer() const noexcept {
    return heap_replacer_;
  }

  memory_resource::heap_resource &heap_resource() noexcept {
    return heap_resource_;
  }

  // feel free to replace with monotonic_buffer_resource for tests or debugging
  using ScriptMemoryResource = memory_resource::unsynchronized_pool_resource;
  ScriptMemoryResource &script_default_resource() noexcept {
    return script_resource_;
  }

private:
  memory_resource::heap_resource heap_resource_;
  ScriptMemoryResource script_resource_;

  memory_resource::synchronized_pool_resource *synchronized_replacer_{nullptr};
  memory_resource::heap_resource *heap_replacer_{nullptr};
};

} // namespace memory_resource
