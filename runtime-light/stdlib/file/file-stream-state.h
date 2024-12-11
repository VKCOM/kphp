// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/file/file-resource.h"
#include "runtime-light/stdlib/file/php-streams/php-streams-context.h"

struct FileStreamInstanceState final : private vk::not_copyable {
  mixed error_number_dummy;
  mixed error_description_dummy;

  PhpStreamInstanceState php_stream_state{};

  static FileStreamInstanceState &get() noexcept;
};
