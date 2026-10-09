//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"

namespace vk {

template<typename T, typename Allocator = std::allocator<T>>
class object_pool : private vk::not_copyable, private std::allocator_traits<Allocator>::template rebind_alloc<std::byte> {
  using byte_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<std::byte>;
  using byte_allocator_traits = std::allocator_traits<byte_allocator>;

  union obj_slot {
    obj_slot* m_next;
    T m_obj;
  };

  struct alignas(alignof(obj_slot)) chunk_header {
    chunk_header* m_next{nullptr};
  };

  std::size_t m_chunk_size{0};
  std::size_t m_chunk_byte_size{0};
  chunk_header* m_head_chunk{nullptr};
  obj_slot* m_head_free_slot{nullptr};

  auto get_byte_allocator_ref() noexcept -> byte_allocator& {
    return *this;
  }

  static auto slots_of(chunk_header* chunk) noexcept -> obj_slot* {
    return reinterpret_cast<obj_slot*>(reinterpret_cast<std::byte*>(chunk) + sizeof(chunk_header));
  }

  auto link_new_chunk() noexcept -> void {
    std::byte* mem{byte_allocator_traits::allocate(get_byte_allocator_ref(), m_chunk_byte_size)};
    m_head_chunk = new (mem) chunk_header{m_head_chunk};

    obj_slot* slots{slots_of(m_head_chunk)};
    slots[0].m_next = nullptr;
    for (size_t i = 1; i < m_chunk_size; ++i) {
      slots[i].m_next = std::addressof(slots[i - 1]);
    }

    m_head_free_slot = std::addressof(slots[m_chunk_size - 1]);
  }

public:
  explicit object_pool(std::size_t chunk_size) noexcept
      : m_chunk_size{chunk_size},
        m_chunk_byte_size{sizeof(chunk_header) + chunk_size * sizeof(obj_slot)} {
    assert(chunk_size > 0);

    link_new_chunk();
  }

  explicit object_pool(std::size_t chunk_size, const Allocator& allocator) noexcept
      : byte_allocator(allocator),
        m_chunk_size{chunk_size},
        m_chunk_byte_size{sizeof(chunk_header) + chunk_size * sizeof(obj_slot)} {
    assert(chunk_size > 0);

    link_new_chunk();
  }

  // All acquired objects must be released before destructor call
  ~object_pool() {
    auto* curr_chunk{m_head_chunk};
    while (curr_chunk != nullptr) {
      auto* next_chunk{curr_chunk->m_next};
      byte_allocator_traits::deallocate(get_byte_allocator_ref(), reinterpret_cast<std::byte*>(curr_chunk), m_chunk_byte_size);
      curr_chunk = next_chunk;
    }
  }

  // All acquired objects must be released
  template<typename... Args>
  auto acquire(Args&&... args) noexcept -> T& {
    if (unlikely(m_head_free_slot == nullptr)) {
      link_new_chunk();
    }

    obj_slot* free_slot{m_head_free_slot};
    m_head_free_slot = m_head_free_slot->m_next;

    byte_allocator_traits::construct(get_byte_allocator_ref(), std::addressof(free_slot->m_obj), std::forward<Args>(args)...);

    return free_slot->m_obj;
  }

  auto release(T& obj) noexcept -> void {
    byte_allocator_traits::destroy(get_byte_allocator_ref(), std::addressof(obj));

    auto* slot{reinterpret_cast<obj_slot*>(std::addressof(obj))};
    slot->m_next = m_head_free_slot;
    m_head_free_slot = slot;
  }
};

} // namespace vk
