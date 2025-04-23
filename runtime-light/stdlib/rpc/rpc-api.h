// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-exceptions.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"
#include "runtime-light/stdlib/rpc/rpc-tl-kphp-request.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::rpc {

struct query_info {
  int64_t id{kphp::rpc::INVALID_QUERY_ID};
  size_t request_size{0};
  double timestamp{0.0};
};

kphp::coro::task<kphp::rpc::query_info> send(string actor, Optional<double> timeout, bool ignore_answer, bool collect_responses_extra_info) noexcept;

namespace rpc_impl {

kphp::coro::task<kphp::rpc::query_info> rpc_tl_query_one_impl(string actor, mixed tl_object, Optional<double> timeout, bool collect_resp_extra_info,
                                                              bool ignore_answer) noexcept;

kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept;

kphp::coro::task<kphp::rpc::query_info> typed_rpc_tl_query_one_impl(string actor, const RpcRequest& rpc_request, Optional<double> timeout,
                                                                    bool collect_responses_extra_info, bool ignore_answer) noexcept;

kphp::coro::task<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory& error_factory) noexcept;

} // namespace rpc_impl

} // namespace kphp::rpc

// === server =====================================================================================

inline bool f$store_int(int64_t v) noexcept {
  if (tl::is_int32_overflow(v)) [[unlikely]] {
    php_warning("Got int32 overflow on storing '%" PRIi64 "', the value will be casted to '%d'", v, static_cast<int32_t>(v));
  }
  tl::i32{.value = static_cast<int32_t>(v)}.store(RpcClientInstanceState::get().rpc_buffer);
  return true;
}

inline bool f$store_long(int64_t v) noexcept {
  tl::i64{.value = v}.store(RpcClientInstanceState::get().rpc_buffer);
  return true;
}

inline bool f$store_float(double v) noexcept {
  tl::f32{.value = static_cast<float>(v)}.store(RpcClientInstanceState::get().rpc_buffer);
  return true;
}

inline bool f$store_double(double v) noexcept {
  tl::f64{.value = v}.store(RpcClientInstanceState::get().rpc_buffer);
  return true;
}

inline bool f$store_string(const string& v) noexcept {
  tl::string{.value = {v.c_str(), v.size()}}.store(RpcClientInstanceState::get().rpc_buffer);
  return true;
}

inline void f$store_raw_vector_double(const array<double>& vector) noexcept {
  const std::string_view vector_view{reinterpret_cast<const char*>(vector.get_const_vector_pointer()), sizeof(double) * vector.count()};
  RpcClientInstanceState::get().rpc_buffer.store_bytes(vector_view);
}

inline int64_t f$fetch_int() noexcept {
  static constexpr auto DEFAULT_VALUE = 0;
  if (tl::i32 val{}; val.fetch(RpcClientInstanceState::get().rpc_buffer)) [[likely]] {
    return static_cast<int64_t>(val.value);
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline int64_t f$fetch_long() noexcept {
  static constexpr int64_t DEFAULT_VALUE = 0;
  if (tl::i64 val{}; val.fetch(RpcClientInstanceState::get().rpc_buffer)) [[likely]] {
    return val.value;
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline double f$fetch_double() noexcept {
  static constexpr double DEFAULT_VALUE = 0.0;
  if (tl::f32 val{}; val.fetch(RpcClientInstanceState::get().rpc_buffer)) [[likely]] {
    return static_cast<double>(val.value);
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline double f$fetch_float() noexcept {
  static constexpr double DEFAULT_VALUE = 0.0;
  if (tl::f64 val{}; val.fetch(RpcClientInstanceState::get().rpc_buffer)) [[likely]] {
    return val.value;
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline string f$fetch_string() noexcept {
  if (tl::string val{}; val.fetch(RpcClientInstanceState::get().rpc_buffer)) [[likely]] {
    return {val.value.data(), static_cast<string::size_type>(val.value.size())};
  }
  THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_string::make());
  return {};
}

inline void f$fetch_raw_vector_double(array<double>& vector, int64_t num_elems) noexcept {
  auto& rpc_buf{RpcClientInstanceState::get().rpc_buffer};
  const auto len_bytes{sizeof(double) * num_elems};
  if (rpc_buf.remaining() < len_bytes) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  vector.memcpy_vector(num_elems, std::next(rpc_buf.data(), rpc_buf.pos()));
  rpc_buf.adjust(len_bytes);
}

inline void f$rpc_clean() noexcept {
  RpcClientInstanceState::get().rpc_buffer.clean();
}

template<typename T>
bool f$rpc_parse(T /*unused*/) {
  php_critical_error("call to unsupported function");
}

// === client =====================================================================================

inline int64_t f$rpc_tl_pending_queries_count() noexcept {
  return RpcClientInstanceState::get().response_waiter_forks.size();
}

// === client untyped =============================================================================

inline kphp::coro::task<array<int64_t>> f$rpc_send_requests(string actor, array<mixed> tl_objects, Optional<double> timeout, bool ignore_answer,
                                                            class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info,
                                                            bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) [[unlikely]] {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  const bool collect_resp_extra_info{!ignore_answer && need_responses_extra_info};
  array<int64_t> query_ids{tl_objects.size()};
  array<kphp::rpc::request_extra_info> req_extra_info_arr{tl_objects.size()};

  for (const auto& it : tl_objects) {
    const auto query_info{co_await kphp::rpc::rpc_impl::rpc_tl_query_one_impl(actor, it.get_value(), timeout, collect_resp_extra_info, ignore_answer)};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), kphp::rpc::request_extra_info{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return std::move(query_ids);
}

inline kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses(array<int64_t> query_ids) noexcept {
  array<array<mixed>> res{query_ids.size()};
  for (const auto& it : query_ids) {
    res.set_value(it.get_key(), co_await kphp::rpc::rpc_impl::rpc_tl_query_result_one_impl(it.get_value()));
  }
  co_return std::move(res);
}

template<class T>
kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses(array<T> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses(array<int64_t>::convert_from(query_ids));
}

inline kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses_synchronously(array<int64_t> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses(std::move(query_ids));
}

template<class T>
kphp::coro::task<array<array<mixed>>> f$rpc_fetch_responses_synchronously(array<T> query_ids) noexcept {
  co_return co_await f$rpc_fetch_responses_synchronously(array<int64_t>::convert_from(query_ids));
}

// === client typed ===============================================================================

template<std::derived_from<C$VK$TL$RpcFunction> rpc_function_t, std::same_as<KphpRpcRequest> rpc_request_t = KphpRpcRequest>
kphp::coro::task<array<int64_t>> f$rpc_send_typed_query_requests(string actor, array<class_instance<rpc_function_t>> query_functions, Optional<double> timeout,
                                                                 bool ignore_answer, class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info,
                                                                 bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) [[unlikely]] {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  const bool collect_resp_extra_info{!ignore_answer && need_responses_extra_info};
  array<int64_t> query_ids{query_functions.size()};
  array<kphp::rpc::request_extra_info> req_extra_info_arr{query_functions.size()};

  for (const auto& it : query_functions) {
    const auto query_info{
        co_await kphp::rpc::rpc_impl::typed_rpc_tl_query_one_impl(actor, rpc_request_t{it.get_value()}, timeout, collect_resp_extra_info, ignore_answer)};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), kphp::rpc::request_extra_info{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return std::move(query_ids);
}

template<std::same_as<int64_t> query_id_t = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_t = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_t>
kphp::coro::task<array<class_instance<C$VK$TL$RpcResponse>>> f$rpc_fetch_typed_responses(array<query_id_t> query_ids) noexcept {
  array<class_instance<C$VK$TL$RpcResponse>> res{query_ids.size()};
  for (const auto& it : query_ids) {
    res.set_value(it.get_key(), co_await kphp::rpc::rpc_impl::typed_rpc_tl_query_result_one_impl(it.get_value(), error_factory_t{}));
  }
  co_return std::move(res);
}

template<std::same_as<int64_t> query_id_t = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_t = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_t>
kphp::coro::task<array<class_instance<C$VK$TL$RpcResponse>>> f$rpc_fetch_typed_responses_synchronously(array<query_id_t> query_ids) noexcept {
  co_return co_await f$rpc_fetch_typed_responses(std::move(query_ids));
}

// === misc =======================================================================================

inline bool f$set_fail_rpc_on_int32_overflow(bool fail_rpc) noexcept {
  RpcServerInstanceState::get().fail_rpc_on_int32_overflow = fail_rpc;
  return true;
}
