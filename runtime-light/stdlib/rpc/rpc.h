// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <memory>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/rpc/rpc_extra_info.h"
#include "runtime-light/tl/tl_kphp_rpc_request.h"
#include "runtime-light/tl/tl_rpc_function.h"

constexpr int64_t RPC_VALID_QUERY_ID_RANGE_START = 0;
constexpr int64_t RPC_INVALID_QUERY_ID = -1;
constexpr int64_t RPC_IGNORED_ANSWER_QUERY_ID = -2;

namespace details {

struct RpcQueryInfo {
  int64_t id{RPC_INVALID_QUERY_ID};
  size_t request_size{0};
  double timestamp{0.0};
};

task_t<RpcQueryInfo> typed_rpc_tl_query_one_impl(string actor, const RpcRequest &rpc_request, double timeout, bool collect_responses_extra_info,
                                                 bool ignore_answer) noexcept;

} // namespace details

// === Rpc Store ==================================================================================

bool f$store_raw(const string &data) noexcept;

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

template<typename F, typename R = KphpRpcRequest>
requires(std::derived_from<F, C$VK$TL$RpcFunction> &&std::same_as<R, KphpRpcRequest>) task_t<array<int64_t>> f$typed_rpc_tl_query(
  string actor, array<class_instance<F>> query_functions, double timeout = -1.0, bool ignore_answer = false,
  class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info = {}, bool need_responses_extra_info = false) noexcept {
  if (ignore_answer && need_responses_extra_info) {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  bool collect_resp_extra_info = !ignore_answer && need_responses_extra_info;
  array<int64_t> query_ids{query_functions.size()};
  array<rpc_request_extra_info_t> req_extra_info_arr{query_functions.size()};

  for (auto it = query_functions.cbegin(); it != query_functions.cend(); ++it) {
    const auto query_info{co_await details::typed_rpc_tl_query_one_impl(actor, R{it.get_value()}, timeout, collect_resp_extra_info, ignore_answer)};

    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), rpc_request_extra_info_t{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }

  co_return query_ids;
}

task_t<array<array<mixed>>> f$rpc_tl_query_result(array<int64_t> query_ids) noexcept;

// === Rpc Misc ===================================================================================

// === Misc =======================================================================================

bool is_int32_overflow(int64_t v) noexcept;

void store_raw_vector_double(const array<double> &vector) noexcept;

void fetch_raw_vector_double(array<double> &vector, int64_t num_elems) noexcept;
