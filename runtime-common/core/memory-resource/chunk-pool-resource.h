// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"
#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include <cstddef>
#include <cstring>

namespace memory_resource {

template<size_t ChunkSize>
class chunk_pool_resource : private vk::not_copyable {
  static_assert(ChunkSize > 0, "size of chunk must be greater than 0");

  struct header {
    std::byte* m_next{nullptr};
  };

  static_assert(ChunkSize >= sizeof(header), "size of chunk too small for intrusive list header");

  std::byte* m_head{nullptr};

  auto init_buffer(void* buffer, size_t buffer_size) noexcept -> void {
    std::byte* curr_chunk{static_cast<std::byte*>(buffer)};
    std::byte* last_chunk{m_head + buffer_size - ChunkSize};
    while (curr_chunk != last_chunk) {
      std::byte* next_chunk = curr_chunk + ChunkSize;
      new (curr_chunk) header{next_chunk};
      curr_chunk = next_chunk;
    }

    new (last_chunk) header{nullptr};
  }

public:
  auto init(void* buffer, size_t buffer_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit() && ChunkSize <= buffer_size && buffer_size % ChunkSize == 0);

    m_head = static_cast<std::byte*>(buffer);
    init_buffer(buffer, buffer_size);
  }

  auto allocate() noexcept -> void* {
    if (unlikely(m_head == nullptr)) {
      return nullptr;
    }

    void* allocated_chunk{m_head};
    m_head = reinterpret_cast<header*>(m_head)->m_next;

    return allocated_chunk;
  }

  auto allocate0() noexcept -> void* {
    void* allocated_chunk{allocate()};
    if (likely(allocated_chunk != nullptr)) {
      memset(allocated_chunk, 0x00, ChunkSize);
    }

    return allocated_chunk;
  }

  auto deallocate(void* mem) noexcept -> void {
    new (mem) header{m_head};
    m_head = reinterpret_cast<std::byte*>(mem);
  }
};

} // namespace memory_resource
