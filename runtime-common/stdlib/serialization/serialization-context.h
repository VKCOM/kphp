// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

class SerializationLibContext final : vk::not_copyable {
public:
  string last_json_processor_error;

  static SerializationLibContext &get() noexcept;
};
