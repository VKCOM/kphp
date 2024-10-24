// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"

struct Response {
  static constexpr int32_t ob_max_buffers{50};
  string_buffer output_buffers[ob_max_buffers];
  int32_t current_buffer{};
};

void f$ob_start(const string &callback = string()) noexcept;

Optional<int64_t> f$ob_get_length() noexcept;

int64_t f$ob_get_level() noexcept;

void f$ob_clean() noexcept;

bool f$ob_end_clean() noexcept;

Optional<string> f$ob_get_clean() noexcept;

string f$ob_get_contents() noexcept;

void f$ob_flush() noexcept;

bool f$ob_end_flush() noexcept;

Optional<string> f$ob_get_flush() noexcept;
