// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdint.h>

namespace ffi {

struct Location {
  int64_t begin;
  int64_t end;
};

} // namespace ffi
