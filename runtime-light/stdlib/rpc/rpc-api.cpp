// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-api.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
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
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"

namespace kphp::rpc {

namespace detail {

constexpr double MAX_TIMEOUT = 86400.0;
constexpr double DEFAULT_TIMEOUT = 0.3;

constexpr auto MAX_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{MAX_TIMEOUT});
constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT});

inline std::chrono::nanoseconds normalize_timeout(std::chrono::nanoseconds timeout) noexcept {
  using namespace std::chrono_literals;
  if (timeout <= 0ns || timeout > kphp::rpc::detail::MAX_TIMEOUT_NS) {
    return kphp::rpc::detail::DEFAULT_TIMEOUT_NS;
  }
  return timeout;
}

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

kphp::coro::task<kphp::rpc::query_info> rpc_tl_query_one_impl(std::string_view actor, mixed tl_object, std::optional<double> opt_timeout,
                                                              bool collect_resp_extra_info, bool ignore_answer) noexcept {
  if (!tl_object.is_array()) [[unlikely]] {
    kphp::log::warning("not an array passed to function rpc_tl_query");
    co_return kphp::rpc::query_info{};
  }

  f$rpc_clean();
  auto rpc_tl_query{store_function(tl_object)}; // THROWING
  // handle exceptions that could arise during store_function
  if (!TlRpcError::transform_exception_into_error_if_possible().empty() || rpc_tl_query.is_null()) [[unlikely]] {
    co_return kphp::rpc::query_info{};
  }

  const auto query_info{co_await kphp::rpc::send_request(actor, opt_timeout, ignore_answer, collect_resp_extra_info)};
  if (!ignore_answer) {
    RpcClientInstanceState::get().response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  co_return query_info;
}

kphp::coro::task<kphp::rpc::query_info> typed_rpc_tl_query_one_impl(std::string_view actor, const RpcRequest& rpc_request, std::optional<double> opt_timeout,
                                                                    bool collect_responses_extra_info, bool ignore_answer) noexcept {
  if (rpc_request.empty()) [[unlikely]] {
    kphp::log::warning("query function is null");
    co_return kphp::rpc::query_info{};
  }

  f$rpc_clean();
  auto fetcher{rpc_request.store_request()}; // THROWING
  // handle exceptions that could arise during store_request
  if (!TlRpcError::transform_exception_into_error_if_possible().empty() || !static_cast<bool>(fetcher)) [[unlikely]] {
    co_return kphp::rpc::query_info{};
  }

  const auto query_info{co_await kphp::rpc::send_request(actor, opt_timeout, ignore_answer, collect_responses_extra_info)};
  if (!ignore_answer) {
    auto rpc_tl_query{make_instance<RpcTlQuery>()};
    rpc_tl_query.get()->result_fetcher = std::move(fetcher);
    rpc_tl_query.get()->tl_function_name = rpc_request.tl_function_name();

    RpcClientInstanceState::get().response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  co_return query_info;
}

kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  int64_t response_waiter_fork_id{kphp::forks::INVALID_ID};

  {
    const auto it_query{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_id{rpc_client_instance_st.response_awaiter_forks.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_query, it_fork_id] noexcept {
      rpc_client_instance_st.response_fetcher_instances.erase(it_query);
      rpc_client_instance_st.response_awaiter_forks.erase(it_fork_id);
    }};

    if (it_query == rpc_client_instance_st.response_fetcher_instances.end() || it_fork_id == rpc_client_instance_st.response_awaiter_forks.end()) [[unlikely]] {
      co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_query->second);
    response_waiter_fork_id = it_fork_id->second;
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

  auto opt_response{co_await kphp::forks::wait<string>(response_waiter_fork_id, MAX_TIMEOUT_NS)};
  if (!opt_response) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"can't find waiter fork"});
  }
  auto response{*std::move(opt_response)};
  if (response.empty()) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

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
  int64_t response_waiter_fork_id{kphp::forks::INVALID_ID};

  {
    const auto it_query{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_id{rpc_client_instance_st.response_awaiter_forks.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_query, it_fork_id] noexcept {
      rpc_client_instance_st.response_fetcher_instances.erase(it_query);
      rpc_client_instance_st.response_awaiter_forks.erase(it_fork_id);
    }};

    if (it_query == rpc_client_instance_st.response_fetcher_instances.end() || it_fork_id == rpc_client_instance_st.response_awaiter_forks.end()) [[unlikely]] {
      co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_query->second);
    response_waiter_fork_id = it_fork_id->second;
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

  auto opt_response{co_await kphp::forks::wait<string>(response_waiter_fork_id, MAX_TIMEOUT_NS)};
  if (!opt_response) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"can't find waiter fork"});
  }
  auto response{*std::move(opt_response)};
  if (response.empty()) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

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

kphp::coro::task<kphp::rpc::query_info> send_request(std::string_view actor, std::optional<double> opt_timeout, bool ignore_answer,
                                                     bool collect_responses_extra_info) noexcept {
  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  const size_t request_size{rpc_server_instance_st.tl_storer.view().size_bytes()};
  const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};

  auto expected_stream{kphp::component::stream::open(actor, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return kphp::rpc::query_info{
        .id = kphp::rpc::INVALID_QUERY_ID, .request_size = rpc_server_instance_st.tl_storer.view().size_bytes(), .timestamp = timestamp};
  }

  auto stream{*std::move(expected_stream)};
  {
    auto tl_storer{std::exchange(rpc_server_instance_st.tl_storer, tl::storer{0})};
    const vk::final_action finalizer{[&tl_storer, &rpc_server_instance_st] noexcept {
      if (tl_storer.capacity() > rpc_server_instance_st.tl_storer.capacity()) {
        std::swap(tl_storer, rpc_server_instance_st.tl_storer);
      }
    }};

    // prepare and send RPC request
    // 'request_buf' will look like this:
    //    [ RpcExtraHeaders (optional) ] [ payload ]
    if (const auto& [opt_new_extra_header, cur_extra_header_size]{kphp::rpc::regularize_extra_headers(tl_storer.view(), ignore_answer)}; opt_new_extra_header) {
      std::span<const std::byte> request_body{tl_storer.view().subspan(cur_extra_header_size)};
      std::span<const std::byte> new_header{reinterpret_cast<const std::byte*>(std::addressof(*opt_new_extra_header)),
                                            sizeof(std::remove_cvref_t<decltype(*opt_new_extra_header)>)};

      if (!co_await stream.write_all(new_header) || !co_await kphp::component::send_request(stream, request_body)) [[unlikely]] {
        co_return kphp::rpc::query_info{.id = kphp::rpc::INVALID_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
      }
    } else if (!co_await kphp::component::send_request(stream, tl_storer.view())) [[unlikely]] {
      co_return kphp::rpc::query_info{.id = kphp::rpc::INVALID_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
    }
  }

  const auto query_id{rpc_client_instance_st.current_query_id++};

  // create response extra info
  if (collect_responses_extra_info) {
    rpc_client_instance_st.rpc_responses_extra_info.emplace(query_id, std::make_pair(response_extra_info_status::not_ready, response_extra_info{0, timestamp}));
  }

  // create fork to wait for RPC response. we need to do it even if 'ignore_answer' is 'true' to make sure
  // that the stream will not be closed too early. otherwise, platform may even not send RPC request
  auto awaiter_task{[](int64_t query_id, kphp::component::stream stream, std::chrono::nanoseconds timeout,
                       bool collect_responses_extra_info) noexcept -> kphp::coro::task<string> {
    string response{};
    auto fetch_task{kphp::component::fetch_response(stream, kphp::component::read_ext::append(response))};
    if (auto expected{co_await kphp::coro::io_scheduler::get().schedule(std::move(fetch_task), timeout)}; !expected) [[unlikely]] {
      response = {};
    }

    // update response extra info if needed
    if (collect_responses_extra_info) {
      auto& extra_info_map{RpcClientInstanceState::get().rpc_responses_extra_info};
      if (const auto it_extra_info{extra_info_map.find(query_id)}; it_extra_info != extra_info_map.end()) [[likely]] {
        const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
        it_extra_info->second.second = std::make_tuple(response.size(), timestamp - std::get<1>(it_extra_info->second.second));
        it_extra_info->second.first = response_extra_info_status::ready;
      } else {
        kphp::log::warning("can't find extra info for RPC query {}", query_id);
      }
    }

    co_return std::move(response);
  }};

  // normalize timeout
  const auto normalized_timeout{kphp::rpc::detail::normalize_timeout(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{opt_timeout.value_or(kphp::rpc::detail::DEFAULT_TIMEOUT)}))};
  // start awaiter fork
  const auto awaiter_fork_id{
      kphp::forks::start(std::invoke(std::move(awaiter_task), query_id, std::move(stream), normalized_timeout, collect_responses_extra_info))};

  if (ignore_answer) {
    co_return kphp::rpc::query_info{.id = kphp::rpc::IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }
  rpc_client_instance_st.response_awaiter_forks.emplace(query_id, awaiter_fork_id);
  co_return kphp::rpc::query_info{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

} // namespace kphp::rpc
