// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-light/streams/stream.h"

struct CLIInstanceInstance final : vk::not_copyable {
  std::optional<kphp::component::stream> output_stream;

  static CLIInstanceInstance& get() noexcept;
};
