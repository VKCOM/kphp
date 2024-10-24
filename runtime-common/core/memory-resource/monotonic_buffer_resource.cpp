// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/memory-resource/monotonic_buffer_resource.h"

namespace memory_resource {

void monotonic_buffer::init(void *buffer, size_t buffer_size) noexcept {
  php_assert(buffer_size <= memory_buffer_limit());
  memory_begin_ = static_cast<char *>(buffer);
  memory_current_ = memory_begin_;
  memory_end_ = memory_begin_ + buffer_size;

  stats_ = MemoryStats{};
  stats_.memory_limit = buffer_size;
}

} // namespace memory_resource
