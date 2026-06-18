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
#include "runtime-light/stdlib/rpc/rpc-request-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
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

namespace {

std::expected<string, std::pair<int32_t, string>> get_rpc_response(int64_t query_id, k2::descriptor rpc_d, bool collect_responses_extra_info) {
  std::expected<size_t, int32_t> first_response_size{k2::rpc_get_response_size(rpc_d)};
  if (!first_response_size) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
  }
  string response{reinterpret_cast<char*>(k2::alloc(*first_response_size)), static_cast<string::size_type>(*first_response_size)};
  std::expected<void, int32_t> new_response_size{k2::rpc_fetch_response(rpc_d, {reinterpret_cast<std::byte*>(response.buffer()), response.size()})};
  if (!new_response_size) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
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

  return {response};
}
} // namespace

kphp::coro::task<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept {
  if (query_id < kphp::rpc::VALID_QUERY_ID_RANGE_START) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_WRONG_QUERY_ID, string{"wrong query_id"});
  }

  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  class_instance<RpcTlQuery> rpc_query{};
  std::optional<kphp::rpc::request_info> opt_rpc_request_info{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_rpc_request_info{rpc_client_instance_st.rpc_requests_infos.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_rpc_request_info] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_rpc_request_info != rpc_client_instance_st.rpc_requests_infos.end()) [[likely]] {
        rpc_client_instance_st.rpc_requests_infos.erase(it_rpc_request_info);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() ||
        it_rpc_request_info == rpc_client_instance_st.rpc_requests_infos.end()) [[unlikely]] {
      co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_rpc_request_info.emplace(std::move(it_rpc_request_info->second));
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

  std::optional<string> opt_response{std::nullopt};

  kphp::log::assertion(opt_rpc_request_info.has_value());
  auto rpc_request_info{*opt_rpc_request_info};
  k2::TimePoint now_instant{};
  k2::instant(std::addressof(now_instant));
  std::chrono::nanoseconds now_ns{now_instant.time_point_ns};
  std::chrono::nanoseconds timeout{rpc_request_info.deadline - now_ns};
  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  switch (co_await m_scheduler.poll(rpc_request_info.rpc_d, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event: {
    std::expected<string, std::pair<int32_t, string>> response_expected{
        get_rpc_response(query_id, rpc_request_info.rpc_d, rpc_request_info.collect_responses_extra_info)};
    if (!response_expected) {
      // TODO std::move ??????????????????????
      std::pair<int32_t, string> error{response_expected.error()};
      co_return TlRpcError::make_error(error.first, error.second);
    }
    opt_response = *response_expected;
    break;
  }
  case kphp::coro::poll_status::closed:
    co_return TlRpcError::make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  case kphp::coro::poll_status::error:
    co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"error fetching rpc response"});
  case kphp::coro::poll_status::timeout:
    co_return TlRpcError::make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }
  if (!opt_response) [[unlikely]] {
    co_return TlRpcError::make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

  auto response{*std::move(opt_response)}; // don't check response's emptyness; will throw if it's empty, indicating a fetch error

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
  std::optional<kphp::rpc::request_info> opt_rpc_request_info{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_rpc_request_info{rpc_client_instance_st.rpc_requests_infos.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_rpc_request_info] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_rpc_request_info != rpc_client_instance_st.rpc_requests_infos.end()) [[likely]] {
        rpc_client_instance_st.rpc_requests_infos.erase(it_rpc_request_info);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() ||
        it_rpc_request_info == rpc_client_instance_st.rpc_requests_infos.end()) [[unlikely]] {
      co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_rpc_request_info.emplace(std::move(it_rpc_request_info->second));
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

  std::optional<string> opt_response{std::nullopt};

  kphp::log::assertion(opt_rpc_request_info.has_value());
  auto rpc_request_info{*opt_rpc_request_info};
  k2::TimePoint now_instant{};
  // TODO call k2::instant once for all sending requests in batch
  k2::instant(std::addressof(now_instant));
  std::chrono::nanoseconds now_ns{now_instant.time_point_ns};
  std::chrono::nanoseconds timeout{rpc_request_info.deadline - now_ns};
  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  switch (co_await m_scheduler.poll(rpc_request_info.rpc_d, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event: {
    std::expected<string, std::pair<int32_t, string>> response_expected{
        get_rpc_response(query_id, rpc_request_info.rpc_d, rpc_request_info.collect_responses_extra_info)};
    if (!response_expected) {
      // TODO std::move ??????????????????????
      std::pair<int32_t, string> error{response_expected.error()};
      co_return error_factory.make_error(error.first, error.second);
    }
    opt_response = *response_expected;
    break;
  }
  case kphp::coro::poll_status::closed:
    co_return error_factory.make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  case kphp::coro::poll_status::error:
    co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"error fetching rpc response"});
  case kphp::coro::poll_status::timeout:
    co_return error_factory.make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }
  if (!opt_response) [[unlikely]] {
    co_return error_factory.make_error(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"});
  }

  auto response{*std::move(opt_response)}; // don't check response's emptiness; will throw if it's empty, indicating a fetch error

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
  auto rpc_d_exp{k2::rpc_send_request(actor, tl_storer.view())};
  if (!rpc_d_exp) {
    return kphp::rpc::query_info{.id = kphp::rpc::INTERNAL_ERROR, .request_size = request_size, .timestamp = timestamp};
  }

  // TODO why is query_id incremented here but not before the request ???
  const auto query_id{rpc_client_instance_st.current_query_id++};

  // create response extra info
  if (collect_responses_extra_info) {
    rpc_client_instance_st.rpc_responses_extra_info.emplace(query_id, std::make_pair(response_extra_info_status::not_ready, response_extra_info{0, timestamp}));
  }

  k2::descriptor rpc_d{*rpc_d_exp};

  static constexpr auto ignore_answer_awaiter_coroutine{[](k2::descriptor rpc_d, std::chrono::milliseconds timeout) noexcept -> kphp::coro::shared_task<> {
    auto fetch_task{kphp::coro::io_scheduler::get().poll(rpc_d, kphp::coro::poll_op::read, timeout)};
    std::ignore = co_await kphp::coro::io_scheduler::get().schedule(std::move(fetch_task), timeout);
  }};

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
  if (ignore_answer) {
    // start ignore answer awaiter task
    // TODO need to move rpc_d ?
    auto ignore_answer_awaiter_task{ignore_answer_awaiter_coroutine(std::move(rpc_d), timeout)};
    kphp::log::assertion(kphp::coro::io_scheduler::get().start(ignore_answer_awaiter_task));

    rpc_client_instance_st.ignore_answer_request_awaiter_tasks.push(std::move(ignore_answer_awaiter_task));
    return kphp::rpc::query_info{.id = kphp::rpc::IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }
  // start awaiter task

  k2::TimePoint now_instant{};
  k2::instant(std::addressof(now_instant));
  std::chrono::nanoseconds now_ns{now_instant.time_point_ns};
  std::chrono::nanoseconds timeout_ns{duration_cast<std::chrono::nanoseconds>(timeout)};
  std::chrono::nanoseconds deadline{now_ns + timeout_ns};

  rpc_client_instance_st.rpc_requests_infos.emplace(query_id, kphp::rpc::request_info{rpc_d, deadline, collect_responses_extra_info});
  return kphp::rpc::query_info{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

} // namespace kphp::rpc
