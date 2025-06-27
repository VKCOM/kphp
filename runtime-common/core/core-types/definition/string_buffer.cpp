// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <memory>
#include <utility>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"

string_buffer::string_buffer(string::size_type buffer_len) noexcept
    : buffer_end(static_cast<char*>(RuntimeAllocator::get().alloc_global_memory(buffer_len))),
      buffer_begin(buffer_end),
      buffer_len(buffer_len) {}

string_buffer::string_buffer(string_buffer&& other) noexcept
    : buffer_end(std::exchange(other.buffer_end, nullptr)),
      buffer_begin(std::exchange(other.buffer_begin, nullptr)),
      buffer_len(std::exchange(other.buffer_len, 0)) {}

string_buffer& string_buffer::operator=(string_buffer&& other) noexcept {
  if (this != std::addressof(other)) {
    RuntimeAllocator::get().free_global_memory(buffer_begin, buffer_len);
    buffer_end = std::exchange(other.buffer_end, nullptr);
    buffer_begin = std::exchange(other.buffer_begin, nullptr);
    buffer_len = std::exchange(other.buffer_len, 0);
  }
  return *this;
}

string_buffer::~string_buffer() noexcept {
  RuntimeAllocator::get().free_global_memory(buffer_begin, buffer_len);
}
