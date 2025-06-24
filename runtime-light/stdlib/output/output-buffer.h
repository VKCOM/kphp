// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <cstdint>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

struct Response {
  // TODO default constructor
  Response() noexcept
      : output_buffers_(1) {
    output_buffers_.reserve(ob_max_buffers);
  }

  int32_t current_buffer_id() const noexcept {
    return current_buffer_;
  }

  string_buffer& current_buffer() noexcept {
    return output_buffers_[current_buffer_];
  }

  bool can_allocate_new_buffer() const noexcept {
    return current_buffer_ + 1 != ob_max_buffers;
  }

  void next_buffer() noexcept {
    assert(current_buffer_ + 1 != ob_max_buffers);

    if (current_buffer_ + 1 == output_buffers_.size()) {
      output_buffers_.resize(next_size(output_buffers_.size()));
    }
    ++current_buffer_;
  }

  void prev_buffer() noexcept {
    --current_buffer_;
  }

  auto& output_buffers() noexcept {
    return output_buffers_;
  }

private:
  static constexpr int32_t ob_max_buffers{50};

  static size_t next_size(size_t old) noexcept {
    assert(old != 0);
    return std::min(old * 2, static_cast<size_t>(ob_max_buffers));
  }

  kphp::stl::vector<string_buffer, kphp::memory::script_allocator> output_buffers_;
  int32_t current_buffer_{};
};

void f$ob_start(const string& callback = string()) noexcept;

Optional<int64_t> f$ob_get_length() noexcept;

int64_t f$ob_get_level() noexcept;

void f$ob_clean() noexcept;

bool f$ob_end_clean() noexcept;

Optional<string> f$ob_get_clean() noexcept;

string f$ob_get_contents() noexcept;

void f$ob_flush() noexcept;

bool f$ob_end_flush() noexcept;

Optional<string> f$ob_get_flush() noexcept;
