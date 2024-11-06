// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

struct FileStreamInstanceState {
  mixed error_number_dummy;
  mixed error_description_dummy;

  static FileStreamInstanceState &get() noexcept;
};
