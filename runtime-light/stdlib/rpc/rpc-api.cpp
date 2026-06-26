// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-api.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "common/rpc-error-codes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/shared-task.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-query-handle.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/stdlib/time/util.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"

namespace kphp::rpc {

namespace detail {

mixed mixed_array_get_value(const mixed& arr, const string& str_key, int64_t num_key) noexcept {
  if (!arr.is_array()) [[unlikely]] {
    return {};
  }

  if (const auto& elem{arr.get_value(num_key)}; !elem.is_null()) {
    return elem;
  }
  if (const auto& elem{arr.get_value(str_key)}; !elem.is_null()) {
    return elem;
  }
  return {};
}

// THROWING
array<mixed> fetch_function_untyped(const class_instance<RpcTlQuery>& rpc_query) noexcept {
  kphp::log::assertion(!rpc_query.is_null());
  if (TlRpcError err{}; TRY_CALL(bool, array<mixed>, err.try_fetch())) {
    return err.make_error();
  }

  auto& cur_query{CurrentTlQuery::get()};
  const vk::final_action finalizer{[&cur_query] noexcept { cur_query.reset(); }};
  cur_query.set_current_tl_function(rpc_query);
  auto fetcher{rpc_query.get()->result_fetcher->extract_untyped_fetcher()};
  kphp::log::assertion(static_cast<bool>(fetcher));

  // TODO: EOF handling
  return TRY_CALL(array<mixed>, array<mixed>, RpcImageState::get().tl_fetch_wrapper(std::move(fetcher)));
}

// THROWING
class_instance<C$VK$TL$RpcResponse> fetch_function_typed(const class_instance<RpcTlQuery>& rpc_query, const RpcErrorFactory& error_factory) noexcept {
  kphp::log::assertion(!rpc_query.is_null());

  auto& cur_query{CurrentTlQuery::get()};
  const vk::final_action finalizer{[&cur_query] noexcept { cur_query.reset(); }};
  cur_query.set_current_tl_function(rpc_query);
  if (TlRpcError err{}; TRY_CALL(bool, class_instance<C$VK$TL$RpcResponse>, err.try_fetch())) {
    return error_factory.make_error(std::move(err));
  }

  // TODO: EOF handling
  return TRY_CALL(class_instance<C$VK$TL$RpcResponse>, class_instance<C$VK$TL$RpcResponse>, rpc_query.get()->result_fetcher->fetch_typed_response());
}

// THROWING
class_instance<RpcTlQuery> store_function(const mixed& tl_object) noexcept {
  auto& cur_query{CurrentTlQuery::get()};
  const vk::final_action finalizer{[&cur_query] noexcept { cur_query.reset(); }};
  const auto& rpc_image_state{RpcImageState::get()};

  const auto fun_name{mixed_array_get_value(tl_object, string{"_"}, 0).to_string()};
  if (!rpc_image_state.tl_storers_ht.has_key(fun_name)) [[unlikely]] {
    return cur_query.raise_storing_error("function \"%s\" not found in tl-scheme", fun_name.c_str()), class_instance<RpcTlQuery>{};
  }

  auto rpc_tl_query{make_instance<RpcTlQuery>()};
  rpc_tl_query.get()->tl_function_name = fun_name;
  cur_query.set_current_tl_function(fun_name);

  const auto& untyped_storer{rpc_image_state.tl_storers_ht.get_value(fun_name)};
  rpc_tl_query.get()->result_fetcher = make_unique_on_script_memory<RpcRequestResultUntyped>(untyped_storer(tl_object));
  CHECK_EXCEPTION(return class_instance<RpcTlQuery>{});
  return rpc_tl_query;
}

kphp::rpc::query_info rpc_tl_query_one_impl(std::string_view actor, mixed tl_object, std::optional<double> opt_timeout, bool collect_resp_extra_info,
                                            bool ignore_answer) noexcept {
  if (!tl_object.is_array()) [[unlikely]] {
    kphp::log::warning("not an array passed to function rpc_tl_query");
    return kphp::rpc::query_info{};
  }

  f$rpc_clean();
  auto rpc_tl_query{store_function(tl_object)}; // THROWING
  // handle exceptions that could arise during store_function
  if (!TlRpcError::transform_exception_into_error_if_possible().empty() || rpc_tl_query.is_null()) [[unlikely]] {
    return kphp::rpc::query_info{};
  }

  const auto query_info{kphp::rpc::send_request(actor, opt_timeout, ignore_answer, collect_resp_extra_info)};
  if (!ignore_answer) {
    RpcClientInstanceState::get().response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  return query_info;
}

kphp::rpc::query_info typed_rpc_tl_query_one_impl(std::string_view actor, const RpcRequest& rpc_request, std::optional<double> opt_timeout,
                                                  bool collect_responses_extra_info, bool ignore_answer) noexcept {
  if (rpc_request.empty()) [[unlikely]] {
    kphp::log::warning("query function is null");
    return kphp::rpc::query_info{};
  }

  f$rpc_clean();
  auto fetcher{rpc_request.store_request()}; // THROWING
  // handle exceptions that could arise during store_request
  if (!TlRpcError::transform_exception_into_error_if_possible().empty() || !static_cast<bool>(fetcher)) [[unlikely]] {
    return kphp::rpc::query_info{};
  }

  const auto query_info{kphp::rpc::send_request(actor, opt_timeout, ignore_answer, collect_responses_extra_info)};
  if (!ignore_answer) {
    auto rpc_tl_query{make_instance<RpcTlQuery>()};
    rpc_tl_query.get()->result_fetcher = std::move(fetcher);
    rpc_tl_query.get()->tl_function_name = rpc_request.tl_function_name();

    RpcClientInstanceState::get().response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  return query_info;
}

kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  std::optional<kphp::rpc::query_handle> opt_rpc_request_handle{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_rpc_request_handle{rpc_client_instance_st.rpc_query_handles.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_rpc_request_handle] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_rpc_request_handle != rpc_client_instance_st.rpc_query_handles.end()) [[likely]] {
        rpc_client_instance_st.rpc_query_handles.erase(it_rpc_request_handle);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() ||
        it_rpc_request_handle == rpc_client_instance_st.rpc_query_handles.end()) [[unlikely]] {
      co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_rpc_request_handle.emplace(std::move(it_rpc_request_handle->second));
  }

  if (rpc_query.is_null()) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"can't use rpc_tl_query_result for non-TL query"});
  }
  if (!rpc_query.get()->result_fetcher || rpc_query.get()->result_fetcher->empty()) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"rpc query has empty result fetcher"});
  }
  if (rpc_query.get()->result_fetcher->is_typed) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"can't get untyped result from typed TL query. Use consistent API for that"});
  }

  kphp::log::assertion(opt_rpc_request_handle.has_value());
  auto response_expected{co_await opt_rpc_request_handle->get_response()};
  if (!response_expected) [[unlikely]] {
    std::pair<int32_t, string> error{std::move(response_expected.error())};
    co_return TlRpcError::make_error(error.first, std::move(error.second));
  }

  auto response{*std::move(response_expected)}; // don't check response's emptiness; will throw if it's empty, indicating a fetch error

  f$rpc_clean();
  RpcServerInstanceState::get().tl_fetcher = tl::fetcher{{reinterpret_cast<const std::byte*>(response.c_str()), response.size()}};
  auto res{fetch_function_untyped(rpc_query)}; // THROWING
  // handle exceptions that could arise during fetch_function_untyped
  if (auto err{TlRpcError::transform_exception_into_error_if_possible()}; !err.empty()) [[unlikely]] {
    co_return std::move(err);
  }
  co_return std::move(res);
}

kphp::coro::task<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory& error_factory) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  std::optional<kphp::rpc::query_handle> opt_rpc_request_handle{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_rpc_request_handle{rpc_client_instance_st.rpc_query_handles.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_rpc_request_handle] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_rpc_request_handle != rpc_client_instance_st.rpc_query_handles.end()) [[likely]] {
        rpc_client_instance_st.rpc_query_handles.erase(it_rpc_request_handle);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() ||
        it_rpc_request_handle == rpc_client_instance_st.rpc_query_handles.end()) [[unlikely]] {
      co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_rpc_request_handle.emplace(std::move(it_rpc_request_handle->second));
  }

  if (rpc_query.is_null()) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"can't use rpc_tl_query_result for non-TL query"});
  }
  if (!rpc_query.get()->result_fetcher || rpc_query.get()->result_fetcher->empty()) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"rpc query has empty result fetcher"});
  }
  if (!rpc_query.get()->result_fetcher->is_typed) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"can't get typed result from untyped TL query. Use consistent API for that"});
  }

  kphp::log::assertion(opt_rpc_request_handle.has_value());
  auto response_expected{co_await opt_rpc_request_handle->get_response()};
  if (!response_expected) [[unlikely]] {
    std::pair<int32_t, string> error{std::move(response_expected.error())};
    co_return error_factory.make_error(error.first, std::move(error.second));
  }

  auto response{*std::move(response_expected)}; // don't check response's emptiness; will throw if it's empty, indicating a fetch error

  f$rpc_clean();
  RpcServerInstanceState::get().tl_fetcher = tl::fetcher{{reinterpret_cast<const std::byte*>(response.c_str()), response.size()}};
  auto res{fetch_function_typed(rpc_query, error_factory)}; // THROWING
  // handle exceptions that could arise during fetch_function_typed
  if (auto err{error_factory.transform_exception_into_error_if_possible()}; !err.is_null()) [[unlikely]] {
    co_return std::move(err);
  }
  co_return std::move(res);
}

} // namespace detail

kphp::rpc::query_info send_request(std::string_view actor, std::optional<double> opt_timeout, bool ignore_answer, bool collect_responses_extra_info) noexcept {
  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  const size_t request_size{rpc_server_instance_st.tl_storer.view().size_bytes()};
  const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};

  auto tl_storer{std::exchange(rpc_server_instance_st.tl_storer, tl::storer{0})};
  const vk::final_action finalizer{[&tl_storer, &rpc_server_instance_st] noexcept {
    if (tl_storer.capacity() > rpc_server_instance_st.tl_storer.capacity()) {
      std::swap(tl_storer, rpc_server_instance_st.tl_storer);
    }
  }};

  const auto query_id{rpc_client_instance_st.current_query_id++};

  // normalize timeout
  using namespace std::chrono_literals;
  static constexpr auto DEFAULT_TIMEOUT{300ms};
  static constexpr auto MIN_TIMEOUT{1ms};
  static constexpr auto MAX_TIMEOUT{std::chrono::duration_cast<std::chrono::milliseconds>(24h)};
  static_assert(MIN_TIMEOUT <= MAX_TIMEOUT, "calling std::clamp will lead to undefined behavior");
  const auto timeout{std::clamp(opt_timeout
                                    .transform([](const double& timeout) noexcept {
                                      return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>{timeout});
                                    })
                                    .value_or(DEFAULT_TIMEOUT),
                                MIN_TIMEOUT, MAX_TIMEOUT)};

  auto query_handle_expected{kphp::rpc::send_and_get_handle(actor, collect_responses_extra_info, ignore_answer, timeout, timestamp, query_id, tl_storer.view())};
  if (!query_handle_expected) {
    return kphp::rpc::query_info{.id = kphp::rpc::INTERNAL_ERROR, .request_size = request_size, .timestamp = timestamp};
  }
  auto query_handle{std::move(*query_handle_expected)};

  if (ignore_answer) {
    return kphp::rpc::query_info{.id = kphp::rpc::IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }

  rpc_client_instance_st.rpc_query_handles.emplace(query_id, std::move(query_handle));
  return kphp::rpc::query_info{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

} // namespace kphp::rpc
