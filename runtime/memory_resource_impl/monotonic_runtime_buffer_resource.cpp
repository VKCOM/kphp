// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/memory-resource/monotonic_buffer_resource.h"

#include <array>

#include "runtime/allocator.h"
#include "runtime/oom_handler.h"
#include "runtime/php_assert.h"

namespace memory_resource {
void monotonic_buffer_resource::critical_dump(void* mem, size_t size) const noexcept {
  std::array<char, 1000> malloc_replacement_stacktrace_buf = {'\0'};
  if (dl::is_malloc_replaced()) {
    const char* descr = "last_malloc_replacement_stacktrace:\n";
    std::strcpy(malloc_replacement_stacktrace_buf.data(), descr);
    dl::write_last_malloc_replacement_stacktrace(malloc_replacement_stacktrace_buf.data() + strlen(descr),
                                                 malloc_replacement_stacktrace_buf.size() - strlen(descr));
  }
  php_critical_error("Found unexpected memory piece:\n"
                     "ptr:                  %p\n"
                     "size:                 %zu\n"
                     "memory_begin:         %p\n"
                     "memory_current:       %p\n"
                     "memory_end:           %p\n"
                     "memory_limit:         %zu\n"
                     "memory_used:          %zu\n"
                     "max_memory_used:      %zu\n"
                     "real_memory_used:     %zu\n"
                     "max_real_memory_used: %zu\n"
                     "is_malloc_replaced:   %d\n"
                     "%s",
                     mem, size, memory_begin_, memory_current_, memory_end_, stats_.memory_limit, stats_.memory_used, stats_.max_memory_used,
                     stats_.real_memory_used, stats_.max_real_memory_used, dl::is_malloc_replaced(), malloc_replacement_stacktrace_buf.data());
}

void monotonic_buffer_resource::raise_oom(size_t size) const noexcept {
  vk::singleton<OomHandler>::get().invoke();

  auto mem_pool_size = stats_.memory_limit;
  size_t mem_available = mem_pool_size - stats_.memory_used;
  if (mem_available < size) {
    // to avoid the confusion, don't report this case as "high fragmentation";
    // this is a genuine OOM case when user requested too much memory
    php_out_of_memory_warning("Can't allocate %zu bytes (out of memory)", size);
  } else {
    // try to add some meaningful info to the OOM error message;
    // buffer_free_space is a percentage of memory we have at our disposal
    // if this value is high, then we have significant memory fragmentation issues;
    // values below 0.1 are very good
    auto fragmentation_rate = static_cast<double>(mem_available - size) / static_cast<double>(mem_pool_size);
    auto buffer_free_space = static_cast<double>(mem_available) / static_cast<double>(mem_pool_size);
    const char* fragmentation_hint = "low fragmentation";
    if (fragmentation_rate >= 0.4) {
      fragmentation_hint = "very high fragmentation";
    } else if (fragmentation_rate >= 0.25) {
      fragmentation_hint = "high fragmentation";
    } else if (fragmentation_rate >= 0.1) {
      fragmentation_hint = "normal fragmentation";
    }
    php_out_of_memory_warning("Can't allocate %zu bytes (%s, buffer free space: %.2f)", size, fragmentation_hint, buffer_free_space);
  }

  raise(SIGUSR2);
}
} // namespace memory_resource
