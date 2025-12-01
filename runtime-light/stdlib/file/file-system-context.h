// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct FileSystemLibConstants final : vk::not_copyable {
  string READ_MODE;
  string WRITE_MODE;
  string APPEND_MODE;
  string READ_PLUS_MODE;
  string WRITE_PLUS_MODE;
  string APPEND_PLUS_MODE;

  FileSystemLibConstants() noexcept;

  static const FileSystemLibConstants& get() noexcept;
};
