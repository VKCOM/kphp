// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct SerializationLibContext final : private vk::not_copyable {
  string last_json_processor_error;

  // msgpack serialization
  size_t instance_depth{0};
  uint32_t serialize_as_float32_{0};
  std::optional<std::array<char, 256>> msgpack_error{std::nullopt};

  void set_msgpack_error(const char* msg) noexcept {
    msgpack_error = std::array<char, 256>();
    strncpy(msgpack_error->begin(), msg, 256);
  }

  void clear_msgpack_error() noexcept {
    msgpack_error = std::nullopt;
  }

  static SerializationLibContext& get() noexcept;
};
