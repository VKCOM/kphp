// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstring>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"
#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {

template<typename SegmentPool>
class segmented_stack_resource : private vk::not_copyable {
  struct segment_list_node {
    std::byte* m_curr{nullptr};
    segment_list_node* m_next{nullptr};
  };

  std::size_t m_segment_size{0};
  SegmentPool m_segment_pool;
  segment_list_node* m_head{nullptr};
  std::byte* m_segment_begin{nullptr};
  std::byte* m_segment_curr{nullptr};

  auto switch_to_new_segment(void* segment) noexcept -> void {
    if (m_head != nullptr) {
      m_head->m_curr = m_segment_curr;
    }

    m_segment_begin = static_cast<std::byte*>(segment) + segment_header_size();
    m_segment_curr = m_segment_begin;

    new (segment) segment_list_node{m_segment_begin, m_head};
    m_head = static_cast<segment_list_node*>(segment);
  }

  auto switch_to_old_segment() noexcept -> void {
    segment_list_node* top_segment{m_head};
    m_head = m_head->m_next;
    m_segment_pool.deallocate(top_segment);

    if (m_head != nullptr) {
      m_segment_begin = reinterpret_cast<std::byte*>(m_head) + segment_header_size();
      m_segment_curr = m_head->m_curr;
    } else {
      m_segment_begin = nullptr;
      m_segment_curr = nullptr;
    }
  }

public:
  auto init(void* buffer, size_t buffer_size, size_t segment_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit() && reinterpret_cast<size_t>(buffer) % alignof(segment_list_node) == 0 && segment_size > 0);

    m_segment_size = segment_size;
    m_segment_pool.init(buffer, buffer_size, m_segment_size + segment_header_size());
  }

  auto allocate(size_t size) noexcept -> void* {
    if (unlikely(size > m_segment_size)) {
      return nullptr;
    }

    if (m_head == nullptr || size > m_segment_size - static_cast<size_t>(m_segment_curr - m_segment_begin)) {
      void* new_segment{m_segment_pool.allocate()};
      if (unlikely(new_segment == nullptr)) {
        return nullptr;
      }

      php_assert(reinterpret_cast<size_t>(new_segment) % alignof(segment_list_node) == 0);

      switch_to_new_segment(new_segment);
    }

    void* allocated{m_segment_curr};
    m_segment_curr += size;

    return allocated;
  }

  auto allocate0(size_t size) noexcept -> void* {
    void* allocated{allocate(size)};
    if (likely(allocated != nullptr)) {
      memset(allocated, 0x00, size);
    }

    return allocated;
  }

  auto deallocate(void* mem, size_t size) noexcept -> void {
    m_segment_curr -= size;

    php_assert(static_cast<std::byte*>(mem) == m_segment_curr);

    if (m_segment_curr == m_segment_begin) {
      switch_to_old_segment();
    }
  }

  auto add_extra_memory(void* buffer, size_t buffer_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit() && reinterpret_cast<size_t>(buffer) % alignof(segment_list_node) == 0);

    m_segment_pool.add_extra_memory(buffer, buffer_size);
  }

  auto get_buffer_list_head() noexcept -> auto* {
    return m_segment_pool.get_buffer_list_head();
  }

  static auto segment_header_size() noexcept -> size_t {
    return sizeof(segment_list_node);
  }
};

} // namespace memory_resource
