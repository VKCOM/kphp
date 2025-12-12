// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-exceptions.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"
#include "runtime-light/stdlib/rpc/rpc-tl-kphp-request.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::rpc {

struct query_info {
  int64_t id{kphp::rpc::INVALID_QUERY_ID};
  size_t request_size{0};
  double timestamp{0.0};
};

kphp::coro::task<kphp::rpc::query_info> send_request(std::string_view actor, std::optional<double> timeout, bool ignore_answer,
                                                     bool collect_responses_extra_info) noexcept;

inline kphp::coro::task<std::expected<void, int32_t>> send_response(std::span<const std::byte> response) noexcept {
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  if (!rpc_server_instance_st.request_stream) [[unlikely]] {
    co_return std::unexpected{k2::errno_enodev};
  }

  auto& request_stream{*rpc_server_instance_st.request_stream};
  if (auto expected{co_await kphp::component::send_response(request_stream, response)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  request_stream.reset(k2::INVALID_PLATFORM_DESCRIPTOR);
  co_return std::expected<void, int32_t>{};
}

namespace detail {

kphp::coro::task<kphp::rpc::query_info> rpc_tl_query_one_impl(std::string_view actor, mixed tl_object, std::optional<double> opt_timeout,
                                                              bool collect_resp_extra_info, bool ignore_answer) noexcept;

kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept;

kphp::coro::task<kphp::rpc::query_info> typed_rpc_tl_query_one_impl(std::string_view actor, const RpcRequest& rpc_request, std::optional<double> opt_timeout,
                                                                    bool collect_responses_extra_info, bool ignore_answer) noexcept;

kphp::coro::task<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory& error_factory) noexcept;

} // namespace detail

} // namespace kphp::rpc

// === server =====================================================================================

inline bool f$store_int(int64_t v) noexcept {
  if (tl::is_int32_overflow(v)) [[unlikely]] {
    kphp::log::warning("integer {} overflows int32, it will be cast to {}", v, static_cast<int32_t>(v));
  }
  tl::i32{.value = static_cast<int32_t>(v)}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_byte(int64_t v) noexcept {
  if (v < 0 || v > 255) [[unlikely]] {
    kphp::log::warning("integer {} overflows uint8, it will be cast to {}", v, static_cast<uint8_t>(v));
  }
  tl::u8{.value = static_cast<uint8_t>(v)}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_long(int64_t v) noexcept {
  tl::i64{.value = v}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_float(double v) noexcept {
  tl::f32{.value = static_cast<float>(v)}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_double(double v) noexcept {
  tl::f64{.value = v}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_string(const string& v) noexcept {
  tl::string{.value = {v.c_str(), v.size()}}.store(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline bool f$store_string2(const string& v) noexcept {
  tl::string{.value = {v.c_str(), v.size()}}.store2(RpcServerInstanceState::get().tl_storer);
  return true;
}

inline void f$store_raw_vector_double(const array<double>& vector) noexcept {
  const std::span<const std::byte> vector_view{reinterpret_cast<const std::byte*>(vector.get_const_vector_pointer()), sizeof(double) * vector.count()};
  RpcServerInstanceState::get().tl_storer.store_bytes(vector_view);
}

inline int64_t f$fetch_byte() noexcept {
  static constexpr auto DEFAULT_VALUE = 0;
  if (tl::u8 val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return static_cast<int64_t>(val.value);
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline int64_t f$fetch_int() noexcept {
  static constexpr auto DEFAULT_VALUE = 0;
  if (tl::i32 val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return static_cast<int64_t>(val.value);
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline int64_t f$fetch_long() noexcept {
  static constexpr int64_t DEFAULT_VALUE = 0;
  if (tl::i64 val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return val.value;
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline double f$fetch_double() noexcept {
  static constexpr double DEFAULT_VALUE = 0.0;
  if (tl::f32 val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return static_cast<double>(val.value);
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline double f$fetch_float() noexcept {
  static constexpr double DEFAULT_VALUE = 0.0;
  if (tl::f64 val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return val.value;
  }
  THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
  return DEFAULT_VALUE;
}

inline string f$fetch_string() noexcept {
  if (tl::string val{}; val.fetch(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return {val.value.data(), static_cast<string::size_type>(val.value.size())};
  }
  THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_string::make());
  return {};
}

inline string f$fetch_string2() noexcept {
  if (tl::string val{}; val.fetch2(RpcServerInstanceState::get().tl_fetcher)) [[likely]] {
    return {val.value.data(), static_cast<string::size_type>(val.value.size())};
  }
  THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_string::make());
  return {};
}

inline void f$fetch_raw_vector_double(array<double>& vector, int64_t num_elems) noexcept {
  auto& tl_fetcher{RpcServerInstanceState::get().tl_fetcher};
  const auto len_bytes{sizeof(double) * num_elems};
  if (tl_fetcher.remaining() < len_bytes) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  vector.memcpy_vector(num_elems, std::next(tl_fetcher.view().data(), tl_fetcher.pos()));
  tl_fetcher.adjust(len_bytes);
}

inline bool f$rpc_clean() noexcept {
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  rpc_server_instance_st.tl_storer.clear();
  rpc_server_instance_st.tl_fetcher = tl::fetcher{rpc_server_instance_st.tl_storer.view()};
  return true;
}

inline bool f$rpc_parse(const string& new_rpc_data) noexcept {
  if (new_rpc_data.size() % sizeof(int32_t) != 0) {
    kphp::log::warning("wrong parameter \"new_rpc_data\" of len {} passed to function rpc_parse", new_rpc_data.size());
    return false;
  }

  const std::span<const std::byte> spn{reinterpret_cast<const std::byte*>(new_rpc_data.c_str()), new_rpc_data.size()};
  RpcServerInstanceState::get().tl_storer.store_bytes(spn);
  return true;
}

inline bool f$rpc_parse(const mixed& new_rpc_data) noexcept {
  if (!new_rpc_data.is_string()) {
    kphp::log::warning("parameter 1 of function rpc_parse must be a string, {} is given", new_rpc_data.get_type_c_str());
    return false;
  }

  return f$rpc_parse(new_rpc_data.to_string());
}

inline bool f$rpc_parse(bool new_rpc_data) noexcept {
  return f$rpc_parse(mixed{new_rpc_data});
}

inline bool f$rpc_parse(const Optional<string>& new_rpc_data) noexcept {
  constexpr auto rpc_parse_lambda{[](const auto& v) noexcept { return f$rpc_parse(v); }};
  return call_fun_on_optional_value(rpc_parse_lambda, new_rpc_data);
}

// f$rpc_server_fetch_request() definition is generated into the tl/rpc_server_fetch_request.cpp file.
// It's composed of:
//    1. fetching magic
//    2. switch over all @kphp functions
//    3. tl_func_state storing inside the CurrentRpcServerQuery
class_instance<C$VK$TL$RpcFunction> f$rpc_server_fetch_request() noexcept;

inline kphp::coro::task<bool> f$store_error(int64_t error_code, string error_msg) noexcept {
  if (tl::is_int32_overflow(error_code)) [[unlikely]] {
    kphp::log::warning("error_code overflows int32, {} will be stored", static_cast<int32_t>(error_code));
  }

  tl::K2RpcResponse rpc_response{.value = tl::k2RpcResponseError{.error_code = tl::i32{.value = static_cast<int32_t>(error_code)},
                                                                 .error = tl::string{.value = {error_msg.c_str(), error_msg.size()}}}};
  tl::storer tls{rpc_response.footprint()};
  rpc_response.store(tls);

  if (auto expected{co_await kphp::forks::id_managed(kphp::rpc::send_response(tls.view()))}; !expected) [[unlikely]] {
    kphp::log::warning("can't store RPC error: {}", expected.error());
  }
  kphp::log::error("store_error called. error_code: {}, error_msg: {}", error_code, error_msg.c_str());
}

inline kphp::coro::task<> f$rpc_server_store_response(class_instance<C$VK$TL$RpcFunctionReturnResult> response) noexcept {
  auto tl_func_base{CurrentRpcServerQuery::get().extract()};
  if (!static_cast<bool>(tl_func_base)) [[unlikely]] {
    co_return kphp::log::warning("can't store RPC response: {}", std::to_underlying(kphp::rpc::error::no_pending_request));
  }

  f$rpc_clean();
  TRY_CALL_VOID_CORO(void, tl_func_base->rpc_server_typed_store(response));
  // as we are in a coroutine, we must own the data to prevent it from being overwritten by another coroutine,
  // so create a TLBuffer owned by this coroutine
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  tl::K2RpcResponse rpc_response{.value = tl::k2RpcResponseHeader{.flags = {}, .extra = {}, .result = rpc_server_instance_st.tl_storer.view()}};
  tl::storer tls{rpc_response.footprint()};
  rpc_response.store(tls);

  if (auto expected{co_await kphp::forks::id_managed(kphp::rpc::send_response(tls.view()))}; !expected) [[unlikely]] {
    kphp::log::warning("can't store RPC response: {}", expected.error());
  }
}

// === client =====================================================================================

inline int64_t f$rpc_tl_pending_queries_count() noexcept {
  return RpcClientInstanceState::get().response_awaiter_tasks.size();
}

// === client untyped =============================================================================

inline kphp::coro::task<array<int64_t>> f$rpc_send_requests(string actor, array<mixed> tl_objects, Optional<double> timeout, bool ignore_answer,
                                                            class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info,
                                                            bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) [[unlikely]] {
    kphp::log::warning("both $ignore_answer and $need_responses_extra_info are 'true'. Metrics won't be collected");
  }

  const bool collect_resp_extra_info{!ignore_answer && need_responses_extra_info};
  array<int64_t> query_ids{tl_objects.size()};
  array<kphp::rpc::request_extra_info> req_extra_info_arr{tl_objects.size()};
  auto opt_timeout{timeout.has_value() ? std::optional<double>{timeout.val()} : std::optional<double>{}};

  for (const auto& it : std::as_const(tl_objects)) {
    const auto query_info{co_await kphp::forks::id_managed(
        kphp::rpc::detail::rpc_tl_query_one_impl({actor.c_str(), actor.size()}, it.get_value(), opt_timeout, collect_resp_extra_info, ignore_answer))};
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
  for (const auto& it : std::as_const(query_ids)) {
    res.set_value(it.get_key(), co_await kphp::forks::id_managed(kphp::rpc::detail::rpc_tl_query_result_one_impl(it.get_value())));
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

template<std::derived_from<C$VK$TL$RpcFunction> rpc_function_type, std::same_as<KphpRpcRequest> rpc_request_type = KphpRpcRequest>
kphp::coro::task<array<int64_t>>
f$rpc_send_typed_query_requests(string actor, array<class_instance<rpc_function_type>> query_functions, Optional<double> timeout, bool ignore_answer,
                                class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info, bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) [[unlikely]] {
    kphp::log::warning("both $ignore_answer and $need_responses_extra_info are 'true'. Metrics won't be collected");
  }

  const bool collect_resp_extra_info{!ignore_answer && need_responses_extra_info};
  array<int64_t> query_ids{query_functions.size()};
  array<kphp::rpc::request_extra_info> req_extra_info_arr{query_functions.size()};
  auto opt_timeout{timeout.has_value() ? std::optional<double>{timeout.val()} : std::optional<double>{}};

  for (const auto& it : std::as_const(query_functions)) {
    const auto query_info{co_await kphp::forks::id_managed(kphp::rpc::detail::typed_rpc_tl_query_one_impl(
        {actor.c_str(), actor.size()}, rpc_request_type{it.get_value()}, opt_timeout, collect_resp_extra_info, ignore_answer))};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), kphp::rpc::request_extra_info{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return std::move(query_ids);
}

template<std::same_as<int64_t> query_id_type = int64_t, std::same_as<RpcResponseErrorFactory> error_factory_type = RpcResponseErrorFactory>
requires std::default_initializable<error_factory_type>
kphp::coro::task<array<class_instance<C$VK$TL$RpcResponse>>> f$rpc_fetch_typed_responses(array<query_id_type> query_ids) noexcept {
  array<class_instance<C$VK$TL$RpcResponse>> res{query_ids.size()};
  for (const auto& it : std::as_const(query_ids)) {
    res.set_value(it.get_key(), co_await kphp::forks::id_managed(kphp::rpc::detail::typed_rpc_tl_query_result_one_impl(it.get_value(), error_factory_type{})));
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
