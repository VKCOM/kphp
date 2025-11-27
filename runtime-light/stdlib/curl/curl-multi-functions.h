// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

inline Optional<array<int64_t>> f$curl_multi_info_read([[maybe_unused]] int64_t /*unused*/,
                                                       [[maybe_unused]] Optional<std::optional<std::reference_wrapper<int64_t>>> /*unused*/ = {}) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}
