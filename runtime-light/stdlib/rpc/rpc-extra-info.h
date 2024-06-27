// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <tuple>

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-core/class-instance/refcountable-php-classes.h"
#include "runtime-core/runtime-core.h"

using rpc_request_extra_info_t = std::tuple<int64_t>;          // tuple(request_size)
using rpc_response_extra_info_t = std::tuple<int64_t, double>; // tuple(response_size, response_time)
enum class rpc_response_extra_info_status_t : uint8_t { NOT_READY, READY };

// TODO: visitors
struct C$KphpRpcRequestsExtraInfo final : public refcountable_php_classes<C$KphpRpcRequestsExtraInfo> /*, private DummyVisitorMethods */ {
  array<rpc_request_extra_info_t> extra_info_arr;

  C$KphpRpcRequestsExtraInfo() = default;
  const char *get_class() const noexcept;
  int get_hash() const noexcept;
};

array<rpc_request_extra_info_t> f$KphpRpcRequestsExtraInfo$$get(class_instance<C$KphpRpcRequestsExtraInfo> v$this) noexcept;

Optional<rpc_response_extra_info_t> f$extract_kphp_rpc_response_extra_info(int64_t query_id) noexcept;
