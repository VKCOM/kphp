// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/server/http/http-server-state.h"

void header(std::string_view header, bool replace, int64_t response_code) noexcept;

inline void f$header(const string &str, bool replace = true, int64_t response_code = HttpStatus::NO_STATUS) noexcept {
  header({str.c_str(), str.size()}, replace, response_code);
}
