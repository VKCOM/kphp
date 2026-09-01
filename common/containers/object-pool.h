//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"

namespace vk {

template<typename T, template<typename> typename Allocator>
class object_pool : private vk::not_copyable, private Allocator<std::byte> {
private:
  using allocator_traits = std::allocator_traits<Allocator<std::byte>>;

  struct object_pool_chunk_header {
    object_pool_chunk_header* m_next{nullptr};
  };

  union object_pool_slot {
    object_pool_slot* m_next;
    T m_obj;
  };

  size_t m_chunk_size{0};
  size_t m_chunk_byte_size{0};
  object_pool_chunk_header* m_head_chunk{nullptr};
  object_pool_slot* m_head_free_slot{nullptr};

  auto get_allocator() noexcept -> Allocator<std::byte>& {
    return *this;
  }

  static auto slots_of(object_pool_chunk_header* chunk) noexcept -> object_pool_slot* {
    return reinterpret_cast<object_pool_slot*>(reinterpret_cast<std::byte*>(chunk) + sizeof(object_pool_chunk_header));
  }

  auto link_new_chunk() noexcept -> void {
    std::byte* mem{allocator_traits::allocate(get_allocator(), m_chunk_byte_size)};
    m_head_chunk = new (mem) object_pool_chunk_header{m_head_chunk};

    object_pool_slot* slots{slots_of(m_head_chunk)};
    slots[0].m_next = nullptr;
    for (size_t i = 1; i < m_chunk_size; ++i) {
      slots[i].m_next = std::addressof(slots[i - 1]);
    }

    m_head_free_slot = std::addressof(slots[m_chunk_size - 1]);
  }

public:
  object_pool() noexcept = default;

  explicit object_pool(Allocator<std::byte> allocator) noexcept
      : Allocator<std::byte>(std::move(allocator)) {}

  auto init(size_t chunk_size) noexcept -> void {
    assert(chunk_size > 0);

    m_chunk_size = chunk_size;
    m_chunk_byte_size = sizeof(object_pool_chunk_header) + chunk_size * sizeof(object_pool_slot);
    link_new_chunk();
  }

  template<typename... Args>
  auto acquire(Args&&... args) noexcept -> T& {
    if (unlikely(m_head_free_slot == nullptr)) {
      link_new_chunk();
    }

    object_pool_slot* free_slot{m_head_free_slot};
    m_head_free_slot = m_head_free_slot->m_next;

    new (free_slot) T(std::forward<Args>(args)...);

    return free_slot->m_obj;
  }

  auto release(T& obj) noexcept -> void {
    obj.~T();
    auto* slot{reinterpret_cast<object_pool_slot*>(std::addressof(obj))};
    slot->m_next = m_head_free_slot;
    m_head_free_slot = slot;
  }

  ~object_pool() {
    auto* curr_chunk{m_head_chunk};
    while (curr_chunk != nullptr) {
      auto* next_chunk{curr_chunk->m_next};
      allocator_traits::deallocate(get_allocator(), reinterpret_cast<std::byte*>(curr_chunk), m_chunk_byte_size);
      curr_chunk = next_chunk;
    }
  }
};

} // namespace vk
