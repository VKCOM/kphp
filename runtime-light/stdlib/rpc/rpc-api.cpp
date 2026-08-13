// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-api.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
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
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-query.h"
#include "runtime-light/stdlib/rpc/rpc-tl-error.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
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

static constexpr size_t RESERVED_HEADER_SIZE{sizeof(kphp::rpc::dest_actor_flags_header)};

// store bytes for `kphp::rpc::dest_actor_flags_header` in RpcServerInstanceState::tl_storer.
// we do this to avoid allocating new buffer for regularized rpc extra headers and copying whole request.
void reserve_header() noexcept {
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  kphp::rpc::dest_actor_flags_header reserved_header{};
  rpc_server_instance_st.tl_storer.store_bytes(
    {reinterpret_cast<const std::byte*>(std::addressof(reserved_header)), RESERVED_HEADER_SIZE});
}

kphp::rpc::query_info rpc_tl_query_one_impl(std::string_view actor, const mixed& tl_object, std::optional<double> opt_timeout, bool collect_resp_extra_info,
                                            bool ignore_answer) noexcept {
  if (!tl_object.is_array()) [[unlikely]] {
    kphp::log::warning("not an array passed to function rpc_tl_query");
    return kphp::rpc::query_info{};
  }

  f$rpc_clean();
  reserve_header();
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
  reserve_header();
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
  std::optional<kphp::coro::shared_task<std::expected<string, int32_t>>> opt_awaiter_task{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_task{rpc_client_instance_st.response_awaiter_tasks.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_fork_task] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_fork_task != rpc_client_instance_st.response_awaiter_tasks.end()) [[likely]] {
        rpc_client_instance_st.response_awaiter_tasks.erase(it_fork_task);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() || it_fork_task == rpc_client_instance_st.response_awaiter_tasks.end())
        [[unlikely]] {
      co_return TlRpcError::make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_awaiter_task.emplace(std::move(it_fork_task->second));
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

  kphp::log::assertion(opt_awaiter_task.has_value());
  auto response_expected{co_await kphp::forks::id_managed(*std::exchange(opt_awaiter_task, std::nullopt))};
  if (!response_expected) [[unlikely]] {
    co_return TlRpcError::make_error(response_expected.error(), string{"can't fetch rpc response"});
  }

  auto response{std::move(*response_expected)}; // don't check response's emptiness; will throw if it's empty, indicating a fetch error

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
  std::optional<kphp::coro::shared_task<std::expected<string, int32_t>>> opt_awaiter_task{};

  {
    const auto it_response_fetcher{rpc_client_instance_st.response_fetcher_instances.find(query_id)};
    const auto it_fork_task{rpc_client_instance_st.response_awaiter_tasks.find(query_id)};
    const vk::final_action finalizer{[&rpc_client_instance_st, it_response_fetcher, it_fork_task] noexcept {
      if (it_response_fetcher != rpc_client_instance_st.response_fetcher_instances.end()) [[likely]] {
        rpc_client_instance_st.response_fetcher_instances.erase(it_response_fetcher);
      }
      if (it_fork_task != rpc_client_instance_st.response_awaiter_tasks.end()) [[likely]] {
        rpc_client_instance_st.response_awaiter_tasks.erase(it_fork_task);
      }
    }};

    if (it_response_fetcher == rpc_client_instance_st.response_fetcher_instances.end() || it_fork_task == rpc_client_instance_st.response_awaiter_tasks.end())
        [[unlikely]] {
      co_return error_factory.make_error(TL_ERROR_INTERNAL, string{"unexpectedly could not find query in pending queries"});
    }
    rpc_query = std::move(it_response_fetcher->second);
    opt_awaiter_task.emplace(std::move(it_fork_task->second));
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

  kphp::log::assertion(opt_awaiter_task.has_value());
  auto response_expected{co_await kphp::forks::id_managed(*std::exchange(opt_awaiter_task, std::nullopt))};
  if (!response_expected) [[unlikely]] {
    co_return error_factory.make_error(response_expected.error(), string{"can't fetch rpc response"});
  }

  auto response{std::move(*response_expected)}; // don't check response's emptiness; will throw if it's empty, indicating a fetch error

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

  const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};

  // We have reserved place for one `kphp::rpc::dest_actor_flags_header` in `rpc_server_instance_st.tl_storer` before storing request and calling `send_request(...)`,
  // so the real serialized request starts after `RESERVED_HEADER_SIZE` bytes in tl storer.
  // We do this to have enough place for regularized header after `kphp::rpc::regularize_extra_headers(...)` call.
  // This optimization helps us avoid allocating and copying the whole request.
  std::span<std::byte> request_buffer{rpc_server_instance_st.tl_storer.view().subspan(detail::RESERVED_HEADER_SIZE)};

  if (const auto& [opt_new_extra_header, cur_extra_header_size]{kphp::rpc::regularize_extra_headers(request_buffer, ignore_answer)};
      opt_new_extra_header) {
    std::span<const std::byte> new_header{reinterpret_cast<const std::byte*>(std::addressof(*opt_new_extra_header)),
                                          sizeof(std::remove_cvref_t<decltype(*opt_new_extra_header)>)};
    std::span<const std::byte> request_body{request_buffer.subspan(cur_extra_header_size)};

    // If `regularize_extra_headers` gave us new header, then we must serialize `new_header` before `request_body`.
    //
    //                               here serialized request (business logic) starts
    //                                                   \/
    // tl_storer was:  |reserved dest-actor-flags-header|  [optional old header] |request-body|
    //
    // We want to serialize |our new header| right before |request-body| :
    //
    // tl_storer will be: ... may be some bytes leaved here ... |our new header| |request-body|

    // we do always have enough bytes for `new_header` before `request_body`, because we have reserved it before `send_request(...)` call.
    size_t new_header_offset{detail::RESERVED_HEADER_SIZE + cur_extra_header_size - new_header.size()};
    std::ranges::copy(new_header, rpc_server_instance_st.tl_storer.view().subspan(new_header_offset).data());
  }

  const size_t request_size{request_buffer.size_bytes()};

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

  auto query_expected{kphp::rpc::query::send(actor, timeout, request_buffer, k2::RpcKind::TL_RPC)};
  if (!query_expected) {
    return kphp::rpc::query_info{.id = kphp::rpc::INVALID_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }

  const auto query_id{rpc_client_instance_st.current_query_id++};

  // create response extra info
  if (collect_responses_extra_info) {
    rpc_client_instance_st.rpc_responses_extra_info.emplace(query_id, std::make_pair(response_extra_info_status::not_ready, response_extra_info{0, timestamp}));
  }

  static constexpr auto awaiter_coroutine{
      [](kphp::rpc::query q, int64_t query_id, bool collect_responses_extra_info) noexcept -> kphp::coro::shared_task<std::expected<string, int32_t>> {
        std::expected<string, int32_t> response_exp{std::in_place};
        const auto response_buffer_provider{[&response_exp](size_t size) noexcept -> std::span<std::byte> {
          response_exp->assign(size, true);
          return {reinterpret_cast<std::byte*>(response_exp->buffer()), size};
        }};

        auto fetch_task{kphp::rpc::query::response(std::move(q), response_buffer_provider)};
        auto fetch_result{co_await kphp::coro::io_scheduler::get().schedule(std::move(fetch_task))};
        if (!fetch_result) [[unlikely]] {
          response_exp = std::unexpected{fetch_result.error()};
        }

        // update response extra info if needed
        if (collect_responses_extra_info) {
          auto& extra_info_map{RpcClientInstanceState::get().rpc_responses_extra_info};
          if (const auto it_extra_info{extra_info_map.find(query_id)}; it_extra_info != extra_info_map.end()) [[likely]] {
            const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
            const auto response_size{response_exp.transform([](const string& response) noexcept { return static_cast<int64_t>(response.size()); }).value_or(0)};
            it_extra_info->second.second = std::make_tuple(response_size, timestamp - std::get<1>(it_extra_info->second.second));
            it_extra_info->second.first = response_extra_info_status::ready;
          } else {
            kphp::log::warning("can't find extra info for RPC query {}", query_id);
          }
        }

        co_return std::move(response_exp);
      }};

  if (ignore_answer) {
    return kphp::rpc::query_info{.id = kphp::rpc::IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }

  // start awaiter task
  auto awaiter_task{awaiter_coroutine(std::move(*query_expected), query_id, collect_responses_extra_info)};
  kphp::log::assertion(kphp::coro::io_scheduler::get().start(awaiter_task));

  rpc_client_instance_st.response_awaiter_tasks.emplace(query_id, std::move(awaiter_task));

  return kphp::rpc::query_info{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

} // namespace kphp::rpc
