// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct SerializationLibContext final : private vk::not_copyable {
  string last_json_processor_error;

  // msgpack serialization
  size_t check_instance_depth{0};
  uint32_t serialize_as_float32_{0};

  static SerializationLibContext &get() noexcept;
};
