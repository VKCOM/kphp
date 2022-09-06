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

void monotonic_buffer_resource::raise_oom(size_t size) const {
  // if we have 100mb and can't allocate 50mb, the frag ratio will be 2.0;
  // values below 1.0 are theoretically possible, but very unlikely;
  // having a frag ratio above 4.0 could be a bad sign
  auto mem_pool_size = static_cast<size_t>(memory_end_ - memory_begin_);
  size_t mem_available = mem_pool_size - stats_.memory_used;
  auto fragmentation_ratio = (double)mem_available / (double)size;
  php_out_of_memory_warning("Can't allocate %zu bytes (fragmentation ration: %.3f)", size, fragmentation_ratio);
  raise(SIGUSR2);
}

} // namespace memory_resource
