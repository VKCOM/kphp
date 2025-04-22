// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"
#include "runtime-common/core/runtime-core.h"

struct SerializationLibContext final : private vk::not_copyable {
  string last_json_processor_error;

  // msgpack serialization
  inline static constexpr size_t MSGPACK_ERROR_BUFFER_SIZE = 256;

  size_t instance_depth{0};
  uint32_t serialize_as_float32_{0};
  std::optional<std::array<char, MSGPACK_ERROR_BUFFER_SIZE>> msgpack_error;

  void set_msgpack_error(vk::string_view msg) noexcept {
    msgpack_error = std::array<char, MSGPACK_ERROR_BUFFER_SIZE>{};
    std::strncpy((*msgpack_error).begin(), msg.data(), MSGPACK_ERROR_BUFFER_SIZE);
  }

  void clear_msgpack_error() noexcept {
    msgpack_error = std::nullopt;
  }

  static SerializationLibContext& get() noexcept;
};
