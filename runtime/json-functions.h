// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

constexpr int64_t JSON_UNESCAPED_UNICODE = 1;
constexpr int64_t JSON_FORCE_OBJECT = 16;
constexpr int64_t JSON_PARTIAL_OUTPUT_ON_ERROR = 512;
constexpr int64_t JSON_AVAILABLE_OPTIONS = JSON_UNESCAPED_UNICODE | JSON_FORCE_OBJECT | JSON_PARTIAL_OUTPUT_ON_ERROR;

bool do_json_encode(const mixed &v, int64_t options, bool simple_encode) noexcept;

Optional<string> f$json_encode(const mixed &v, int64_t options = 0, bool simple_encode = false) noexcept;

string f$vk_json_encode_safe(const mixed &v, bool simple_encode = true) noexcept;

mixed f$json_decode(const string &v, bool assoc = false) noexcept;

inline Optional<string> f$vk_json_encode(const mixed &v) noexcept {
  return f$json_encode(v, 0, true);
}
