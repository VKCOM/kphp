// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct FileSystemInstanceState final : private vk::not_copyable {
  mixed error_number_dummy;
  mixed error_description_dummy;

  FileSystemInstanceState() noexcept = default;

  static FileSystemInstanceState& get() noexcept;
};
