// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

struct CurlComponentContext {
  int64_t curl_multi_info_read_msgs_in_queue_stub{};

  static CurlComponentContext &get() noexcept;
};
