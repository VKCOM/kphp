// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cassert>
#include <cinttypes>
#include <cstddef>

#include "common/mixin/not_copyable.h"

namespace memory_resource {

class alignas(8) extra_memory_pool : vk::not_copyable {
public:
  explicit extra_memory_pool(size_t buffer_size) :
    buffer_size_(buffer_size) {
  }

  uint8_t *memory_begin() {
    return reinterpret_cast<uint8_t *>(this + 1);
  }

  bool is_memory_from_this_pool(const void *mem, size_t mem_size) noexcept {
    return memory_begin() <= static_cast<const uint8_t *>(mem) &&
           static_cast<const uint8_t *>(mem) + mem_size <= memory_begin() + get_pool_payload_size();
  }

  static size_t get_pool_payload_size(size_t buffer_size) noexcept {
    return buffer_size - sizeof(extra_memory_pool);
  }

  size_t get_pool_payload_size() const noexcept {
    return get_pool_payload_size(buffer_size_);
  }

  size_t get_buffer_size() const noexcept {
    return buffer_size_;
  }

  extra_memory_pool *next_in_chain{nullptr};

private:
  const size_t buffer_size_{0};
};

class extra_memory_raw_bucket : vk::not_copyable {
public:
  void init(uint8_t *raw_memory, size_t buffers_count, size_t buffer_size) noexcept {
    raw_memory_ = raw_memory;
    buffers_count_ = buffers_count;
    buffer_size_ = buffer_size;
  }

  size_t buffers_count() const noexcept {
    return buffers_count_;
  }

  void *get_extra_pool_raw(size_t index) const noexcept {
    assert(index < buffers_count_);
    return raw_memory_ + index * buffer_size_;
  }

  static int get_bucket(const extra_memory_pool &pool) noexcept {
    return __builtin_ctzll(pool.get_buffer_size()) - 20;
  }

  static size_t get_size_by_bucket(size_t id) noexcept {
    return 1 << (20 + id);
  }

private:
  uint8_t *raw_memory_{nullptr};
  size_t buffers_count_{0};
  size_t buffer_size_{0};
};

template<size_t N>
size_t distribute_memory(std::array<extra_memory_raw_bucket, N> &extra_memory, size_t initial_count, uint8_t *raw, size_t size) noexcept {
  size_t left_memory = size;
  for (size_t i = 0; i != extra_memory.size(); ++i) {
    // (2^i) MB
    const size_t buffer_size = extra_memory_raw_bucket::get_size_by_bucket(i);
    size_t buffers_count = left_memory / buffer_size;
    if (i + 1 != extra_memory.size()) {
      // (initial_count / 2^i)
      buffers_count = std::min(std::max(initial_count >> i, size_t{1}), buffers_count);
      const size_t left_memory_for_next = left_memory - buffers_count * buffer_size;
      const size_t next_buffer_size = buffer_size << 1;
      buffers_count += left_memory_for_next % next_buffer_size >= buffer_size;
    }

    left_memory -= buffers_count * buffer_size;
    extra_memory[i].init(raw, buffers_count, buffer_size);
    raw += buffers_count * buffer_size;
  }
  return left_memory;
}

} // namespace memory_resource
