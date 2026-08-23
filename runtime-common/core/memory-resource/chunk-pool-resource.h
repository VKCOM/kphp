// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstring>
#include <new>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"
#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {

struct buffer_list_node {
  buffer_list_node* next_in_chain{nullptr};
};

class chunk_pool_resource : private vk::not_copyable {
  struct chunk_free_list_node {
    chunk_free_list_node* m_next{nullptr};
  };

  std::size_t m_chunk_size{0};
  chunk_free_list_node* m_head_chunk{nullptr};
  buffer_list_node* m_head_buffer{nullptr};

  auto init_buffer(void* buffer, size_t buffer_size) noexcept -> void {
    new (buffer) buffer_list_node{m_head_buffer};

    std::byte* first_chunk{static_cast<std::byte*>(buffer) + sizeof(buffer_list_node)};

    size_t payload_size{buffer_size - sizeof(buffer_list_node)};
    size_t used_buffer_size{payload_size - payload_size % m_chunk_size};

    std::byte* curr_chunk{first_chunk};
    std::byte* last_chunk{curr_chunk + used_buffer_size - m_chunk_size};
    while (curr_chunk != last_chunk) {
      std::byte* next_chunk = curr_chunk + m_chunk_size;
      new (curr_chunk) chunk_free_list_node{reinterpret_cast<chunk_free_list_node*>(next_chunk)};
      curr_chunk = next_chunk;
    }

    new (last_chunk) chunk_free_list_node{m_head_chunk};

    m_head_chunk = reinterpret_cast<chunk_free_list_node*>(first_chunk);
  }

public:
  auto init(void* buffer, size_t buffer_size, size_t chunk_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit() && buffer_size >= chunk_size + sizeof(buffer_list_node) &&
               reinterpret_cast<size_t>(buffer) % alignof(buffer_list_node) == 0 && chunk_size >= sizeof(chunk_free_list_node) &&
               chunk_size % alignof(chunk_free_list_node) == 0);

    m_chunk_size = chunk_size;
    init_buffer(buffer, buffer_size);
    m_head_buffer = static_cast<buffer_list_node*>(buffer);
  }

  auto allocate() noexcept -> void* {
    if (unlikely(m_head_chunk == nullptr)) {
      return nullptr;
    }

    void* allocated_chunk{m_head_chunk};
    m_head_chunk = m_head_chunk->m_next;

    return allocated_chunk;
  }

  auto allocate0() noexcept -> void* {
    void* allocated_chunk{allocate()};
    if (likely(allocated_chunk != nullptr)) {
      memset(allocated_chunk, 0x00, m_chunk_size);
    }

    return allocated_chunk;
  }

  auto deallocate(void* mem) noexcept -> void {
    m_head_chunk = new (mem) chunk_free_list_node{m_head_chunk};
  }

  auto add_extra_memory(void* buffer, size_t buffer_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit() && buffer_size >= m_chunk_size + sizeof(buffer_list_node) &&
               reinterpret_cast<size_t>(buffer) % alignof(buffer_list_node) == 0);

    init_buffer(buffer, buffer_size);
    m_head_buffer = static_cast<buffer_list_node*>(buffer);
  }

  auto get_buffer_list_head() const noexcept -> buffer_list_node* {
    return m_head_buffer;
  }
};

} // namespace memory_resource
