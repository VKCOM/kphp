// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"

struct CurlInstanceState final : private vk::not_copyable {
  int64_t curl_multi_info_read_msgs_in_queue_stub{};

  CurlInstanceState() noexcept = default;

  static CurlInstanceState& get() noexcept;
};
