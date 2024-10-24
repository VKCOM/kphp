// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <tuple>
#include <cstdint>

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime/dummy-visitor-methods.h"

using rpc_request_extra_info_t = std::tuple<std::int64_t>; // tuple(request_size)
using rpc_response_extra_info_t = std::tuple<std::int64_t, double>; // tuple(response_size, response_time)
enum class rpc_response_extra_info_status_t : std::uint8_t { NOT_READY, READY };

extern array<std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>> rpc_responses_extra_info_map;

struct C$KphpRpcRequestsExtraInfo final : public refcountable_php_classes<C$KphpRpcRequestsExtraInfo>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  array<rpc_request_extra_info_t> extra_info_arr_;

  C$KphpRpcRequestsExtraInfo() = default;

  const char *get_class() const noexcept {
    return R"(KphpRpcRequestsExtraInfo)";
  }

  int get_hash() const noexcept {
    return static_cast<std::int32_t>(vk::std_hash(vk::string_view(C$KphpRpcRequestsExtraInfo::get_class())));
  }
};

array<rpc_request_extra_info_t> f$KphpRpcRequestsExtraInfo$$get(class_instance<C$KphpRpcRequestsExtraInfo> v$this);

Optional<rpc_response_extra_info_t> f$extract_kphp_rpc_response_extra_info(std::int64_t resumable_id);
