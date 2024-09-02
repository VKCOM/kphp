// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"
#include "runtime-light/stdlib/rpc/rpc-tl-kphp-request.h"

constexpr int64_t RPC_VALID_QUERY_ID_RANGE_START = 0;
constexpr int64_t RPC_INVALID_QUERY_ID = -1;
constexpr int64_t RPC_IGNORED_ANSWER_QUERY_ID = -2;

namespace rpc_impl_ {

struct RpcQueryInfo {
  int64_t id{RPC_INVALID_QUERY_ID};
  size_t request_size{0};
  double timestamp{0.0};
};

task_t<RpcQueryInfo> typed_rpc_tl_query_one_impl(string actor, const RpcRequest &rpc_request, double timeout, bool collect_responses_extra_info,
                                                 bool ignore_answer) noexcept;

task_t<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory &error_factory) noexcept;

} // namespace rpc_impl_

// === Rpc Store ==================================================================================

bool f$store_int(int64_t v) noexcept;

bool f$store_long(int64_t v) noexcept;

bool f$store_float(double v) noexcept;

bool f$store_double(double v) noexcept;

bool f$store_string(const string &v) noexcept;

// === Rpc Fetch ==================================================================================

int64_t f$fetch_int() noexcept;

int64_t f$fetch_long() noexcept;

double f$fetch_double() noexcept;

double f$fetch_float() noexcept;

string f$fetch_string() noexcept;

// === Rpc Query ==================================================================================

task_t<array<int64_t>> f$rpc_tl_query(string actor, array<mixed> tl_objects, double timeout = -1.0, bool ignore_answer = false,
                                      class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info = {}, bool need_responses_extra_info = false) noexcept;

template<std::derived_from<C$VK$TL$RpcFunction> rpc_function_t, std::same_as<KphpRpcRequest> rpc_request_t = KphpRpcRequest>
task_t<array<int64_t>> f$typed_rpc_tl_query(string actor, array<class_instance<rpc_function_t>> query_functions, double timeout = -1.0,
                                            bool ignore_answer = false, class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info = {},
                                            bool need_responses_extra_info = false) noexcept {
  if (ignore_answer && need_responses_extra_info) {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  bool collect_resp_extra_info = !ignore_answer && need_responses_extra_info;
  array<int64_t> query_ids{query_functions.size()};
  array<rpc_request_extra_info_t> req_extra_info_arr{query_functions.size()};

  for (const auto &it : query_functions) {
    const auto query_info{
      co_await rpc_impl_::typed_rpc_tl_query_one_impl(actor, rpc_request_t{it.get_value()}, timeout, collect_resp_extra_info, ignore_answer)};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), rpc_request_extra_info_t{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return query_ids;
}

task_t<array<array<mixed>>> f$rpc_tl_query_result(array<int64_t> query_ids) noexcept;

template<std::same_as<int64_t> query_id_t = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_t = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_t> task_t<array<class_instance<C$VK$TL$RpcResponse>>>
f$typed_rpc_tl_query_result(array<query_id_t> query_ids) noexcept {
  array<class_instance<C$VK$TL$RpcResponse>> res{query_ids.size()};
  for (const auto &it : query_ids) {
    res.set_value(it.get_key(), co_await rpc_impl_::typed_rpc_tl_query_result_one_impl(it.get_value(), error_factory_t{}));
  }
  co_return res;
}

// === Rpc Misc ===================================================================================

void f$rpc_clean() noexcept;

// === Misc =======================================================================================

bool is_int32_overflow(int64_t v) noexcept;

void store_raw_vector_double(const array<double> &vector) noexcept;

void fetch_raw_vector_double(array<double> &vector, int64_t num_elems) noexcept;
