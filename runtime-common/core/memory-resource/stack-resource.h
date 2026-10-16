// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstring>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {

class stack_resource : private vk::not_copyable {
  std::byte* m_begin{nullptr};
  std::byte* m_curr{nullptr};
  std::byte* m_end{nullptr};

public:
  auto init(void* buffer, size_t buffer_size) noexcept -> void {
    php_assert(buffer_size <= memory_buffer_limit());

    m_begin = static_cast<std::byte*>(buffer);
    m_curr = m_begin;
    m_end = m_curr + buffer_size;
  }

  auto allocate(size_t size) noexcept -> void* {
    if (size > static_cast<size_t>(m_end - m_curr)) {
      return nullptr;
    }

    void* allocated{m_curr};
    m_curr += size;

    return allocated;
  }

  auto allocate0(size_t size) noexcept -> void* {
    void* allocated{allocate(size)};
    if (allocated != nullptr) {
      memset(allocated, 0x00, size);
    }

    return allocated;
  }

  auto deallocate(void* mem, size_t size) noexcept -> void {
    php_assert(m_curr - static_cast<std::byte*>(mem) == size);
    m_curr -= size;
  }

  auto memory_begin() noexcept -> void* {
    return m_begin;
  }
};

} // namespace memory_resource
