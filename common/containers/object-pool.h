//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <memory>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"

namespace vk {

namespace details {

template<typename T>
union object_pool_slot {
  object_pool_slot* m_next;
  T m_obj;
};

template<typename T, size_t ChunkSize>
struct object_pool_chunk {
  object_pool_chunk* m_next{nullptr};
  object_pool_slot<T> m_slots[ChunkSize];
};

} // namespace details

template<typename T, size_t ChunkSize, template<typename> typename Allocator>
class object_pool : private vk::not_copyable, private Allocator<vk::details::object_pool_chunk<T, ChunkSize>> {
private:
  static_assert(ChunkSize > 0, "ChunkSize must be greater than 0");

  using allocator_traits = std::allocator_traits<Allocator<vk::details::object_pool_chunk<T, ChunkSize>>>;

  vk::details::object_pool_chunk<T, ChunkSize>* m_head_chunk{nullptr};
  vk::details::object_pool_slot<T>* m_head_free_slot{nullptr};

  auto get_allocator() noexcept -> Allocator<vk::details::object_pool_chunk<T, ChunkSize>>& {
    return *this;
  }

  auto link_new_chunk() noexcept -> void {
    vk::details::object_pool_chunk<T, ChunkSize>* chunk{allocator_traits::allocate(get_allocator(), 1)};
    chunk->m_next = m_head_chunk;
    m_head_chunk = chunk;
    vk::details::object_pool_slot<T>* slots{chunk->m_slots};
    slots->m_next = nullptr;
    for (size_t i = 1; i < ChunkSize; ++i) {
      (slots + i)->m_next = (slots + i - 1);
    }

    m_head_free_slot = slots + ChunkSize - 1;
  }

public:
  object_pool() noexcept {
    link_new_chunk();
  }

  explicit object_pool(Allocator<vk::details::object_pool_chunk<T, ChunkSize>> allocator) noexcept
      : Allocator<vk::details::object_pool_chunk<T, ChunkSize>>(std::move(allocator)) {
    link_new_chunk();
  }

  template<typename... Args>
  auto acquire(Args&&... args) noexcept -> T& {
    if (unlikely(m_head_free_slot == nullptr)) {
      link_new_chunk();
    }

    vk::details::object_pool_slot<T>* free_slot{m_head_free_slot};
    m_head_free_slot = m_head_free_slot->m_next;

    new (free_slot) T(std::forward<Args>(args)...);

    return free_slot->m_obj;
  }

  auto release(T& obj) noexcept -> void {
    obj.~T();
    auto* slot{reinterpret_cast<vk::details::object_pool_slot<T>*>(std::addressof(obj))};
    slot->m_next = m_head_free_slot;
    m_head_free_slot = slot;
  }

  ~object_pool() {
    auto* curr_chunk{m_head_chunk};
    while (curr_chunk != nullptr) {
      auto* next_chunk{curr_chunk->m_next};
      allocator_traits::deallocate(get_allocator(), curr_chunk, 1);
      curr_chunk = next_chunk;
    }
  }
};

} // namespace vk
