// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

struct SystemComponentContext {
  int64_t result_code_dummy{};

  static SystemComponentContext &get() noexcept;
};


