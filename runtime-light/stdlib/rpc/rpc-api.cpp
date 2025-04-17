// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-api.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "common/containers/final_action.h"
#include "common/rpc-error-codes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"

namespace kphp::rpc {

namespace rpc_impl {

constexpr double MAX_TIMEOUT_S = 86400.0;
constexpr double DEFAULT_TIMEOUT_S = 0.3;
constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT_S});

inline std::chrono::nanoseconds normalize_timeout(double timeout_s) noexcept {
  if (timeout_s <= 0 || timeout_s > kphp::rpc::rpc_impl::MAX_TIMEOUT_S) {
    return kphp::rpc::rpc_impl::DEFAULT_TIMEOUT_NS;
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{timeout_s});
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
  php_assert(!rpc_query.is_null());
  if (TlRpcError err{}; TRY_CALL(decltype(err.try_fetch()), array<mixed>, err.try_fetch())) [[unlikely]] {
    return err.make_error();
  }

  auto& cur_query{CurrentTlQuery::get()};
  cur_query.set_current_tl_function(rpc_query);
  auto fetcher{rpc_query.get()->result_fetcher->extract_untyped_fetcher()};
  php_assert(fetcher);

  auto res{RpcImageState::get().tl_fetch_wrapper(std::move(fetcher))};
  cur_query.reset();
  // TODO: EOF handling
  return res;
}

// THROWING
class_instance<C$VK$TL$RpcResponse> fetch_function_typed(const class_instance<RpcTlQuery>& rpc_query, const RpcErrorFactory& error_factory) noexcept {
  php_assert(!rpc_query.is_null());

  auto& cur_query{CurrentTlQuery::get()};
  cur_query.set_current_tl_function(rpc_query);
  if (TlRpcError err{}; TRY_CALL(decltype(err.try_fetch()), class_instance<C$VK$TL$RpcResponse>, err.try_fetch())) [[unlikely]] {
    return error_factory.make_error(std::move(err));
  }

  auto res{rpc_query.get()->result_fetcher->fetch_typed_response()};
  cur_query.reset();
  // TODO: EOF handling
  return res;
}

// THROWING
class_instance<RpcTlQuery> store_function(const mixed& tl_object) noexcept {
  auto& cur_query{CurrentTlQuery::get()};
  const auto& rpc_image_state{RpcImageState::get()};

  const auto fun_name{mixed_array_get_value(tl_object, string{"_"}, 0).to_string()};
  if (!rpc_image_state.tl_storers_ht.has_key(fun_name)) [[unlikely]] {
    TRY_CALL_VOID(class_instance<RpcTlQuery>, cur_query.raise_storing_error("Function \"%s\" not found in tl-scheme", fun_name.c_str()));
  }

  auto rpc_tl_query{make_instance<RpcTlQuery>()};
  rpc_tl_query.get()->tl_function_name = fun_name;
  cur_query.set_current_tl_function(fun_name);

  const auto& untyped_storer{rpc_image_state.tl_storers_ht.get_value(fun_name)};
  rpc_tl_query.get()->result_fetcher = make_unique_on_script_memory<RpcRequestResultUntyped>(untyped_storer(tl_object));
  cur_query.reset();
  return rpc_tl_query;
}

kphp::coro::task<kphp::rpc::query_info> rpc_tl_query_one_impl(string actor, mixed tl_object, Optional<double> timeout, bool collect_resp_extra_info,
                                                              bool ignore_answer) noexcept {
  if (!tl_object.is_array()) [[unlikely]] {
    php_warning("not an array passed to function rpc_tl_query");
    co_return kphp::rpc::query_info{};
  }

  auto& rpc_instance_st{RpcInstanceState::get()};

  rpc_instance_st.rpc_buffer.clean();
  auto rpc_tl_query{store_function(tl_object)}; // THROWING
  // get rid of possible exceptions
  if (!TlRpcError::transform_exception_into_error_if_possible().empty()) [[unlikely]] {
    co_return kphp::rpc::query_info{};
  }
  if (rpc_tl_query.is_null()) [[unlikely]] {
    php_warning("could not store rpc request");
    co_return kphp::rpc::query_info{};
  }

  const auto query_info{co_await kphp::rpc::send(actor, timeout, ignore_answer, collect_resp_extra_info)};
  if (!ignore_answer) {
    rpc_instance_st.response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  co_return query_info;
}

kphp::coro::task<kphp::rpc::query_info> typed_rpc_tl_query_one_impl(string actor, const RpcRequest& rpc_request, Optional<double> timeout,
                                                                    bool collect_responses_extra_info, bool ignore_answer) noexcept {
  if (rpc_request.empty()) [[unlikely]] {
    php_warning("query function is null");
    co_return kphp::rpc::query_info{};
  }

  auto& rpc_instance_st{RpcInstanceState::get()};

  rpc_instance_st.rpc_buffer.clean();
  auto fetcher{rpc_request.store_request()}; // THROWING
  // get rid of possible exceptions
  if (!TlRpcError::transform_exception_into_error_if_possible().empty()) [[unlikely]] {
    co_return kphp::rpc::query_info{};
  }
  if (!static_cast<bool>(fetcher)) [[unlikely]] {
    php_warning("could not store rpc request");
    co_return kphp::rpc::query_info{};
  }

  const auto query_info{co_await kphp::rpc::send(actor, timeout, ignore_answer, collect_responses_extra_info)};
  if (!ignore_answer) {
    auto rpc_tl_query{make_instance<RpcTlQuery>()};
    rpc_tl_query.get()->result_fetcher = std::move(fetcher);
    rpc_tl_query.get()->tl_function_name = rpc_request.tl_function_name();

    rpc_instance_st.response_fetcher_instances.emplace(query_info.id, std::move(rpc_tl_query));
  }
  co_return query_info;
}

// THROWING
kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_instance_st{RpcInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  int64_t response_waiter_fork_id{kphp::forks::INVALID_ID};

  {
    const auto it_query{rpc_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_id{rpc_instance_st.response_waiter_forks.find(query_id)};
    const vk::final_action finalizer{[&rpc_instance_st, it_query, it_fork_id] noexcept {
      rpc_instance_st.response_fetcher_instances.erase(it_query);
      rpc_instance_st.response_waiter_forks.erase(it_fork_id);
    }};

    if (it_query == rpc_instance_st.response_fetcher_instances.end() || it_fork_id == rpc_instance_st.response_waiter_forks.end()) [[unlikely]] {
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

  auto opt_data{co_await f$wait<Optional<string>>(response_waiter_fork_id, MAX_TIMEOUT_S)};
  if (!opt_data.has_value()) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"can't find waiter fork"});
  }
  if (opt_data.val().empty()) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

  auto data{std::move(opt_data.val())};
  rpc_instance_st.rpc_buffer.clean();
  rpc_instance_st.rpc_buffer.store_bytes({data.c_str(), static_cast<size_t>(data.size())});
  auto res{fetch_function_untyped(rpc_query)}; // THROWING
  // get rid of possible exceptions
  if (auto err{TlRpcError::transform_exception_into_error_if_possible()}; !err.empty()) [[unlikely]] {
    res = std::move(err);
  }
  co_return std::move(res);
}

kphp::coro::task<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory& error_factory) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_instance_st{RpcInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  int64_t response_waiter_fork_id{kphp::forks::INVALID_ID};

  {
    const auto it_query{rpc_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_id{rpc_instance_st.response_waiter_forks.find(query_id)};
    const vk::final_action finalizer{[&rpc_instance_st, it_query, it_fork_id] noexcept {
      rpc_instance_st.response_fetcher_instances.erase(it_query);
      rpc_instance_st.response_waiter_forks.erase(it_fork_id);
    }};

    if (it_query == rpc_instance_st.response_fetcher_instances.end() || it_fork_id == rpc_instance_st.response_waiter_forks.end()) [[unlikely]] {
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

  auto opt_data{co_await f$wait<Optional<string>>(response_waiter_fork_id, MAX_TIMEOUT_S)};
  if (!opt_data.has_value()) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"can't find waiter fork"});
  }
  if (opt_data.val().empty()) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

  auto data{std::move(opt_data.val())};
  rpc_instance_st.rpc_buffer.clean();
  rpc_instance_st.rpc_buffer.store_bytes({data.c_str(), static_cast<size_t>(data.size())});
  auto res{fetch_function_typed(rpc_query, error_factory)}; // THROWING
  // get rid of possible exceptions
  if (auto err{error_factory.transform_exception_into_error_if_possible()}; !err.is_null()) [[unlikely]] {
    res = std::move(err);
  }
  co_return std::move(res);
}

} // namespace rpc_impl

kphp::coro::task<kphp::rpc::query_info> send(string actor, Optional<double> timeout, bool ignore_answer, bool collect_responses_extra_info) noexcept {
  auto& rpc_instance_st{RpcInstanceState::get()};

  // prepare RPC request
  string request_buf{};
  size_t request_size{rpc_instance_st.rpc_buffer.size()};
  // 'request_buf' will look like this:
  //    [ RpcExtraHeaders (optional) ] [ payload ]
  if (const auto& [opt_new_extra_header, cur_extra_header_size]{kphp::rpc::regularize_extra_headers(
          {reinterpret_cast<const std::byte*>(rpc_instance_st.rpc_buffer.data()), rpc_instance_st.rpc_buffer.size()}, ignore_answer)};
      opt_new_extra_header.has_value()) {
    const auto new_extra_header{*opt_new_extra_header};
    const auto new_extra_header_size{sizeof(std::remove_cvref_t<decltype(new_extra_header)>)};
    request_size = request_size - cur_extra_header_size + new_extra_header_size;

    request_buf.reserve_at_least(request_size)
        .append(reinterpret_cast<const char*>(std::addressof(new_extra_header)), new_extra_header_size)
        .append(rpc_instance_st.rpc_buffer.data() + cur_extra_header_size, rpc_instance_st.rpc_buffer.size() - cur_extra_header_size);
  } else {
    request_buf.append(rpc_instance_st.rpc_buffer.data(), request_size);
  }

  // send RPC request
  const auto query_id{rpc_instance_st.current_query_id++};
  const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
  auto comp_query{co_await f$component_client_send_request(actor, std::move(request_buf))};
  if (comp_query.is_null()) [[unlikely]] {
    php_warning("can't send rpc query to %s", actor.c_str());
    co_return kphp::rpc::query_info{.id = kphp::rpc::INVALID_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }

  // create response extra info
  if (collect_responses_extra_info) {
    rpc_instance_st.rpc_responses_extra_info.emplace(query_id, std::make_pair(response_extra_info_status::not_ready, response_extra_info{0, timestamp}));
  }

  // normalize timeout
  const auto timeout_ns{kphp::rpc::rpc_impl::normalize_timeout(timeout.has_value() ? timeout.val() : kphp::rpc::rpc_impl::DEFAULT_TIMEOUT_S)};
  // create fork to wait for RPC response. we need to do it even if 'ignore_answer' is 'true' to make sure
  // that the stream will not be closed too early. otherwise, platform may even not send RPC request
  auto waiter_task{std::invoke(
      [](int64_t query_id, auto comp_query, std::chrono::nanoseconds timeout, bool collect_responses_extra_info) noexcept -> kphp::coro::task<string> {
        auto fetch_task{f$component_client_fetch_response(std::move(comp_query))};
        const auto response{(co_await wait_with_timeout_t{fetch_task.operator co_await(), timeout}).value_or(string{})};
        // update response extra info if needed
        if (collect_responses_extra_info) {
          auto& extra_info_map{RpcInstanceState::get().rpc_responses_extra_info};
          if (const auto it_extra_info{extra_info_map.find(query_id)}; it_extra_info != extra_info_map.end()) [[likely]] {
            const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
            it_extra_info->second.second = std::make_tuple(response.size(), timestamp - std::get<1>(it_extra_info->second.second));
            it_extra_info->second.first = response_extra_info_status::ready;
          } else {
            php_warning("can't find extra info for RPC query %" PRId64, query_id);
          }
        }
        co_return std::move(response);
      },
      query_id, std::move(comp_query), timeout_ns, collect_responses_extra_info)};
  // start waiter fork
  const auto waiter_fork_id{co_await start_fork_t{std::move(waiter_task)}};

  if (ignore_answer) {
    co_return kphp::rpc::query_info{.id = kphp::rpc::IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }
  rpc_instance_st.response_waiter_forks.emplace(query_id, waiter_fork_id);
  co_return kphp::rpc::query_info{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

} // namespace kphp::rpc
