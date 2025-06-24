// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

struct Response {
  static constexpr int32_t ob_max_buffers{50};

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
    ++current_buffer_;
  }

  void prev_buffer() noexcept {
    --current_buffer_;
  }

  auto& output_buffers() noexcept {
    return output_buffers_; 
  }

private:
  // kphp::stl::vector<string_buffer, kphp::memory::script_allocator> b;
  string_buffer output_buffers_[ob_max_buffers];
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
