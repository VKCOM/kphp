// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <time.h>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"

inline string f$strftime(const string& format, int64_t timestamp = std::numeric_limits<int64_t>::min()) noexcept {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm broken_down_time{};
  const time_t timestamp_t = timestamp;
  localtime_r(std::addressof(timestamp_t), std::addressof(broken_down_time));

  if (!strftime(StringLibContext::get().static_buf.get(), StringLibContext::STATIC_BUFFER_LENGTH, format.c_str(), std::addressof(broken_down_time))) {
    return {};
  }

  return string{StringLibContext::get().static_buf.get()};
}
