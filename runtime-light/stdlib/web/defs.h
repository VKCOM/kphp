// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web/web-state.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

enum class TransferBackend : uint8_t { CURL = 1 };

using SimpleTransfer = uint64_t;
using PropertyId = uint64_t;
using PropertyValue = mixed;

struct SimpleTransferConfig {
  kphp::stl::unordered_map<PropertyId, PropertyValue, kphp::memory::script_allocator> properties{};
};

struct Response {
  string headers;
  string body;
};

struct Error {
  int64_t code;
  std::optional<string> description;
};

struct Ok {};

} // namespace kphp::web