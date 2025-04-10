// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstring>

inline const char* kbasename(const char* path) noexcept {
  if (const char* file_short = strrchr(path, '/')) {
    if (file_short[1]) {
      path = file_short + 1;
    }
  }
  return path;
}
