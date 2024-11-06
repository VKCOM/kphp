// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/curl-context.h"

inline Optional<array<int64_t>> f$curl_multi_info_read(int64_t, int64_t & = CurlInstanceState::get().curl_multi_info_read_msgs_in_queue_stub) {
  php_critical_error("call to unsupported function : curl_multi_info_read");
}

inline bool f$curl_setopt_array(int64_t, const array<mixed> &) noexcept {
  php_critical_error("call to unsupported function : curl_multi_info_read");
}
