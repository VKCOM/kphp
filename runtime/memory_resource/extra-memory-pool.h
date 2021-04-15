// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>

#include "common/mixin/not_copyable.h"

namespace memory_resource {

class alignas(8) extra_memory_pool : vk::not_copyable {
public:
  explicit extra_memory_pool(size_t memory_real_size) :
    payload_size_(get_pool_payload_size(memory_real_size)) {
    assert(memory_real_size >= sizeof(extra_memory_pool));
  }

  uint8_t *memory_begin() {
    return reinterpret_cast<uint8_t *>(this + 1);
  }

  uint8_t *memory_end() {
    return memory_begin() + payload_size_;
  }

  bool is_memory_from_this_pool(const void *mem, size_t mem_size) noexcept {
    return memory_begin() <= static_cast<const uint8_t *>(mem) &&
           static_cast<const uint8_t *>(mem) + mem_size <= memory_end();
  }

  static size_t get_pool_payload_size(size_t memory_real_size) noexcept {
    return memory_real_size - sizeof(extra_memory_pool);
  }

  size_t get_pool_payload_size() const noexcept {
    return payload_size_;
  }

  extra_memory_pool *next_in_chain{nullptr};

private:
  const size_t payload_size_{0};
};

class extra_memory_pool_storage : vk::not_copyable {
public:
  void init(uint8_t *raw_memory, uint32_t buffers_count, uint32_t buffer_size) noexcept {
    raw_memory_ = raw_memory;
    buffers_count_ = buffers_count;
    buffer_size_ = buffer_size;
    for (uint32_t i = 0; i != buffers_count; ++i) {
      new(get_extra_pool(i)) memory_resource::extra_memory_pool{buffer_size_};
    }
  }

  uint32_t buffers_count() const noexcept {
    return buffers_count_;
  }

  size_t get_buffer_payload_size() const noexcept {
    return memory_resource::extra_memory_pool::get_pool_payload_size(buffer_size_);
  }

  memory_resource::extra_memory_pool *get_extra_pool(size_t index) const noexcept {
    assert(index < buffers_count_);
    return reinterpret_cast<memory_resource::extra_memory_pool *>(raw_memory_ + index * buffer_size_);
  }

private:
  uint8_t *raw_memory_{nullptr};
  size_t buffers_count_{0};
  size_t buffer_size_{0};
};

template<size_t N>
size_t distribute_memory(std::array<extra_memory_pool_storage, N> &extra_memory, size_t initial_count, uint8_t *raw, size_t size) noexcept {
  size_t left_memory = size;
  for (size_t i = 0; i != extra_memory.size(); ++i) {
    // (2^i) MB
    const size_t buffer_size = 1 << (20 + i);
    // (initial_count / 2^i)
    size_t buffers_count = std::min(initial_count >> i, left_memory / buffer_size);
    left_memory -= buffers_count * buffer_size;
    const size_t next_buffer_size = buffer_size << 1;
    if (left_memory % next_buffer_size >= buffer_size) {
      // last buffer
      if (i + 1 == extra_memory.size()) {
        buffers_count += left_memory / buffer_size;
      } else {
        ++buffers_count;
      }
    }
    extra_memory[i].init(raw, buffers_count, buffer_size);
    raw += buffers_count * buffer_size;
  }
  return left_memory;
}

} // namespace memory_resource
