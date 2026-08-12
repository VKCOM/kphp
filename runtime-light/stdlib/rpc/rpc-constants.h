// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

namespace kphp::rpc {

inline constexpr int64_t VALID_QUERY_ID_RANGE_START = 1;
inline constexpr int64_t INVALID_QUERY_ID = 0;
inline constexpr int64_t IGNORED_ANSWER_QUERY_ID = -1;

enum class error : int32_t {
  // stream errors
  invalid_stream = -999,
  not_rpc_stream,

  // operation errors
  write_failed = -1999,

  // RPC errors
  no_pending_request = -2999,
};

} // namespace kphp::rpc
