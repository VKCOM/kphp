// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <tuple>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

namespace kphp::rpc {

using rpc_request_extra_info_t = std::tuple<int64_t>;          // tuple(request_size)
using rpc_response_extra_info_t = std::tuple<int64_t, double>; // tuple(response_size, response_time)
enum class rpc_response_extra_info_status_t : uint8_t { NOT_READY, READY };

} // namespace kphp::rpc

struct C$KphpRpcRequestsExtraInfo final : public refcountable_php_classes<C$KphpRpcRequestsExtraInfo>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  array<kphp::rpc::rpc_request_extra_info_t> extra_info_arr;

  C$KphpRpcRequestsExtraInfo() = default;

  constexpr const char* get_class() const noexcept {
    return R"(KphpRpcRequestsExtraInfo)";
  }

  constexpr int32_t get_hash() const noexcept {
    std::string_view name_view{C$KphpRpcRequestsExtraInfo::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
};

inline array<kphp::rpc::rpc_request_extra_info_t> f$KphpRpcRequestsExtraInfo$$get(const class_instance<C$KphpRpcRequestsExtraInfo>& v$this) noexcept {
  return v$this.get()->extra_info_arr;
}

Optional<kphp::rpc::rpc_response_extra_info_t> f$extract_kphp_rpc_response_extra_info(int64_t query_id) noexcept;
