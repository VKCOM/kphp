// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"
#include "runtime-light/stdlib/rpc/rpc-tl-kphp-request.h"

namespace kphp::rpc {

namespace rpc_impl {

struct RpcQueryInfo {
  int64_t id{kphp::rpc::INVALID_QUERY_ID};
  size_t request_size{0};
  double timestamp{0.0};
};

kphp::coro::task<RpcQueryInfo> typed_rpc_tl_query_one_impl(string actor, const RpcRequest& rpc_request, Optional<double> timeout,
                                                           bool collect_responses_extra_info, bool ignore_answer) noexcept;

kphp::coro::task<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory& error_factory) noexcept;

} // namespace rpc_impl

} // namespace kphp::rpc

// === Rpc Store ==================================================================================

bool f$store_int(int64_t v) noexcept;

bool f$store_long(int64_t v) noexcept;

bool f$store_float(double v) noexcept;

bool f$store_double(double v) noexcept;

bool f$store_string(const string& v) noexcept;

// === Rpc Fetch ==================================================================================

int64_t f$fetch_int() noexcept;

int64_t f$fetch_long() noexcept;

double f$fetch_double() noexcept;

double f$fetch_float() noexcept;

string f$fetch_string() noexcept;

// === Rpc Query ==================================================================================

kphp::coro::task<array<int64_t>> f$rpc_send_requests(string actor, array<mixed> tl_objects, Optional<double> timeout, bool ignore_answer,
                                                     class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info, bool need_responses_extra_info) noexcept;

template<std::derived_from<C$VK$TL$RpcFunction> rpc_function_t, std::same_as<KphpRpcRequest> rpc_request_t = KphpRpcRequest>
kphp::coro::task<array<int64_t>> f$rpc_send_typed_query_requests(string actor, array<class_instance<rpc_function_t>> query_functions, Optional<double> timeout,
                                                                 bool ignore_answer, class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info,
                                                                 bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  bool collect_resp_extra_info = !ignore_answer && need_responses_extra_info;
  array<int64_t> query_ids{query_functions.size()};
  array<rpc_request_extra_info_t> req_extra_info_arr{query_functions.size()};

  for (const auto& it : query_functions) {
    const auto query_info{
        co_await kphp::rpc::rpc_impl::typed_rpc_tl_query_one_impl(actor, rpc_request_t{it.get_value()}, timeout, collect_resp_extra_info, ignore_answer)};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), rpc_request_extra_info_t{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return query_ids;
}

kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses(array<int64_t> query_ids) noexcept;

template<class T>
kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses(array<T> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses(array<int64_t>::convert_from(query_ids));
}

template<std::same_as<int64_t> query_id_t = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_t = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_t>
kphp::coro::task<array<class_instance<C$VK$TL$RpcResponse>>> f$rpc_fetch_typed_responses(array<query_id_t> query_ids) noexcept {
  array<class_instance<C$VK$TL$RpcResponse>> res{query_ids.size()};
  for (const auto& it : query_ids) {
    res.set_value(it.get_key(), co_await kphp::rpc::rpc_impl::typed_rpc_tl_query_result_one_impl(it.get_value(), error_factory_t{}));
  }
  co_return res;
}

inline kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses_synchronously(array<int64_t> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses(std::move(query_ids));
}

template<class T>
kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses_synchronously(array<T> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses_synchronously(array<int64_t>::convert_from(query_ids));
}

template<std::same_as<int64_t> query_id_t = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_t = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_t>
kphp::coro::task<array<class_instance<C$VK$TL$RpcResponse>>> f$rpc_fetch_typed_responses_synchronously(array<query_id_t> query_ids) noexcept {
  co_return co_await f$rpc_fetch_typed_responses(std::move(query_ids));
}

// === Rpc Misc ===================================================================================

inline void f$rpc_clean() noexcept {
  RpcInstanceState::get().rpc_buffer.clean();
}

inline int64_t f$rpc_tl_pending_queries_count() noexcept {
  return RpcInstanceState::get().response_waiter_forks.size();
}

inline bool f$set_fail_rpc_on_int32_overflow(bool fail_rpc) noexcept {
  RpcInstanceState::get().fail_rpc_on_int32_overflow = fail_rpc;
  return true;
}

// === Misc =======================================================================================

bool is_int32_overflow(int64_t v) noexcept;

void store_raw_vector_double(const array<double>& vector) noexcept;

void fetch_raw_vector_double(array<double>& vector, int64_t num_elems) noexcept;

template<typename T>
bool f$rpc_parse(T /*unused*/) {
  php_critical_error("call to unsupported function");
}
