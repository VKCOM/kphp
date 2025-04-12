// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"

struct SystemInstanceState final : private vk::not_copyable {
  int64_t result_code_dummy{};
  Optional<int64_t> rest_index_dummy;

  static SystemInstanceState& get() noexcept;
};
