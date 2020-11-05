// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_resource/monotonic_buffer_resource.h"

namespace memory_resource {

void monotonic_buffer::init(void *buffer, size_t buffer_size) noexcept {
  php_assert(buffer_size <= memory_buffer_limit());
  memory_begin_ = static_cast<char *>(buffer);
  memory_current_ = memory_begin_;
  memory_end_ = memory_begin_ + buffer_size;

  stats_ = MemoryStats{};
  stats_.memory_limit = buffer_size;
}

void monotonic_buffer::critical_dump(void *mem, size_t size) const noexcept {
  php_critical_error(
    "Found unexpected memory piece:\n"
    "ptr:                  %p\n"
    "size:                 %zu\n"
    "memory_begin:         %p\n"
    "memory_current:       %p\n"
    "memory_end:           %p\n"
    "memory_limit:         %zu\n"
    "memory_used:          %zu\n"
    "max_memory_used:      %zu\n"
    "real_memory_used:     %zu\n"
    "max_real_memory_used: %zu\n",
    mem, size, memory_begin_, memory_current_, memory_end_,
    stats_.memory_limit,
    stats_.memory_used, stats_.max_memory_used,
    stats_.real_memory_used, stats_.max_real_memory_used);
}

} // namespace memory_resource
