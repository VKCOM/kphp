// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-api.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include "common/algorithms/find.h"
#include "common/rpc-error-codes.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"
#include "runtime-light/streams/interface.h"
#include "runtime-light/tl/tl-core.h"

namespace rpc_impl_ {

constexpr int32_t MAX_TIMEOUT_S = 86400;

mixed mixed_array_get_value(const mixed &arr, const string &str_key, int64_t num_key) noexcept {
  if (!arr.is_array()) {
    return {};
  }

  if (const auto &elem{arr.get_value(num_key)}; !elem.is_null()) {
    return elem;
  }
  if (const auto &elem{arr.get_value(str_key)}; !elem.is_null()) {
    return elem;
  }
  return {};
}

array<mixed> make_fetch_error(string &&error_msg, int32_t error_code) {
  array<mixed> res;
  res.set_value(string{"__error", 7}, std::move(error_msg));
  res.set_value(string{"__error_code", 12}, error_code);
  return res;
}

array<mixed> fetch_function_untyped(const class_instance<RpcTlQuery> &rpc_query) noexcept {
  php_assert(!rpc_query.is_null());
  CurrentTlQuery::get().set_current_tl_function(rpc_query);
  auto fetcher{rpc_query.get()->result_fetcher->extract_untyped_fetcher()};
  php_assert(fetcher);

  const auto res{RpcImageState::get().tl_fetch_wrapper(std::move(fetcher))};
  // TODO: exception handling
  // TODO: EOF handling
  return res;
}

class_instance<C$VK$TL$RpcResponse> fetch_function_typed(const class_instance<RpcTlQuery> &rpc_query, const RpcErrorFactory &error_factory) noexcept {
  php_assert(!rpc_query.is_null());
  CurrentTlQuery::get().set_current_tl_function(rpc_query);
  // check if the response is error
  if (const auto rpc_error{error_factory.fetch_error_if_possible()}; !rpc_error.is_null()) {
    return rpc_error;
  }
  const auto res{rpc_query.get()->result_fetcher->fetch_typed_response()};
  // TODO: exception handling
  // TODO: EOF handling
  return res;
}

class_instance<RpcTlQuery> store_function(const mixed &tl_object) noexcept {
  auto &cur_query{CurrentTlQuery::get()};
  const auto &rpc_image_state{RpcImageState::get()};

  const auto fun_name{mixed_array_get_value(tl_object, string{"_"}, 0).to_string()}; // TODO: constexpr ctor for string{"_"}
  if (!rpc_image_state.tl_storers_ht.has_key(fun_name)) {
    cur_query.raise_storing_error("Function \"%s\" not found in tl-scheme", fun_name.c_str());
    return {};
  }

  auto rpc_tl_query{make_instance<RpcTlQuery>()};
  rpc_tl_query.get()->tl_function_name = fun_name;

  cur_query.set_current_tl_function(fun_name);
  const auto &untyped_storer = rpc_image_state.tl_storers_ht.get_value(fun_name);
  rpc_tl_query.get()->result_fetcher = make_unique_on_script_memory<RpcRequestResultUntyped>(untyped_storer(tl_object));
  cur_query.reset();
  return rpc_tl_query;
}

task_t<RpcQueryInfo> rpc_send_impl(string actor, double timeout, bool ignore_answer) noexcept {
  if (timeout <= 0 || timeout > MAX_TIMEOUT_S) { // TODO: handle timeouts
    //    timeout = conn.get()->timeout_ms * 0.001;
  }

  auto &rpc_ctx = RpcComponentContext::get();
  string request_buf{};
  size_t request_size{rpc_ctx.rpc_buffer.size()};

  // 'request_buf' will look like this:
  //    [ RpcExtraHeaders (optional) ] [ payload ]
  if (const auto [opt_new_extra_header, cur_extra_header_size]{regularize_extra_headers(rpc_ctx.rpc_buffer.data(), ignore_answer)}; opt_new_extra_header) {
    const auto new_extra_header{opt_new_extra_header.value()};
    const auto new_extra_header_size{sizeof(std::decay_t<decltype(new_extra_header)>)};
    request_size = request_size - cur_extra_header_size + new_extra_header_size;

    request_buf.append(reinterpret_cast<const char *>(&new_extra_header), new_extra_header_size);
    request_buf.append(rpc_ctx.rpc_buffer.data() + cur_extra_header_size, rpc_ctx.rpc_buffer.size() - cur_extra_header_size);
  } else {
    request_buf.append(rpc_ctx.rpc_buffer.data(), request_size);
  }

  // get timestamp before co_await to also count the time we were waiting for runtime to resume this coroutine
  const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};

  auto comp_query{co_await f$component_client_send_query(actor, request_buf)};
  if (comp_query.is_null()) {
    php_error("could not send rpc query to %s", actor.c_str());
    co_return RpcQueryInfo{.id = RPC_INVALID_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }

  if (ignore_answer) { // TODO: wait for answer in a separate coroutine and keep returning RPC_IGNORED_ANSWER_QUERY_ID
    co_return RpcQueryInfo{.id = RPC_IGNORED_ANSWER_QUERY_ID, .request_size = request_size, .timestamp = timestamp};
  }
  const auto query_id{rpc_ctx.current_query_id++};
  rpc_ctx.pending_component_queries.emplace(query_id, std::move(comp_query));
  co_return RpcQueryInfo{.id = query_id, .request_size = request_size, .timestamp = timestamp};
}

task_t<RpcQueryInfo> rpc_tl_query_one_impl(string actor, mixed tl_object, double timeout, bool collect_resp_extra_info, bool ignore_answer) noexcept {
  auto &rpc_ctx{RpcComponentContext::get()};

  if (!tl_object.is_array()) {
    rpc_ctx.current_query.raise_storing_error("not an array passed to function rpc_tl_query");
    co_return RpcQueryInfo{};
  }

  rpc_ctx.rpc_buffer.clean();
  auto rpc_tl_query{store_function(tl_object)}; // TODO: exception handling
  if (rpc_tl_query.is_null()) {
    co_return RpcQueryInfo{};
  }

  const auto query_info{co_await rpc_send_impl(actor, timeout, ignore_answer)};
  if (!ignore_answer) {
    rpc_ctx.pending_rpc_queries.emplace(query_info.id, std::move(rpc_tl_query));
  }
  if (collect_resp_extra_info) {
    rpc_ctx.rpc_responses_extra_info.emplace(query_info.id,
                                             std::make_pair(rpc_response_extra_info_status_t::NOT_READY, rpc_response_extra_info_t{0, query_info.timestamp}));
  }

  co_return query_info;
}

task_t<RpcQueryInfo> typed_rpc_tl_query_one_impl(string actor, const RpcRequest &rpc_request, double timeout, bool collect_responses_extra_info,
                                                 bool ignore_answer) noexcept {
  auto &rpc_ctx{RpcComponentContext::get()};

  if (rpc_request.empty()) {
    rpc_ctx.current_query.raise_storing_error("query function is null");
    co_return RpcQueryInfo{};
  }

  rpc_ctx.rpc_buffer.clean();
  auto fetcher{rpc_request.store_request()};
  if (!static_cast<bool>(fetcher)) {
    rpc_ctx.current_query.raise_storing_error("could not store rpc request");
    co_return RpcQueryInfo{};
  }

  const auto query_info{co_await rpc_send_impl(actor, timeout, ignore_answer)};
  if (!ignore_answer) {
    auto rpc_tl_query{make_instance<RpcTlQuery>()};
    rpc_tl_query.get()->result_fetcher = std::move(fetcher);
    rpc_tl_query.get()->tl_function_name = rpc_request.tl_function_name();

    rpc_ctx.pending_rpc_queries.emplace(query_info.id, std::move(rpc_tl_query));
  }
  if (collect_responses_extra_info) {
    rpc_ctx.rpc_responses_extra_info.emplace(query_info.id,
                                             std::make_pair(rpc_response_extra_info_status_t::NOT_READY, rpc_response_extra_info_t{0, query_info.timestamp}));
  }

  co_return query_info;
}

task_t<array<mixed>> rpc_tl_query_result_one_impl(int64_t query_id) noexcept {
  if (query_id < RPC_VALID_QUERY_ID_RANGE_START) {
    co_return make_fetch_error(string{"wrong query_id"}, TL_ERROR_WRONG_QUERY_ID);
  }

  auto &rpc_ctx{RpcComponentContext::get()};
  class_instance<RpcTlQuery> rpc_query{};
  class_instance<C$ComponentQuery> component_query{};

  {
    const auto it_rpc_query{rpc_ctx.pending_rpc_queries.find(query_id)};
    const auto it_component_query{rpc_ctx.pending_component_queries.find(query_id)};

    vk::final_action finalizer{[&rpc_ctx, it_rpc_query, it_component_query]() {
      rpc_ctx.pending_rpc_queries.erase(it_rpc_query);
      rpc_ctx.pending_component_queries.erase(it_component_query);
    }};

    if (it_rpc_query == rpc_ctx.pending_rpc_queries.end() || it_component_query == rpc_ctx.pending_component_queries.end()) {
      co_return make_fetch_error(string{"unexpectedly could not find query in pending queries"}, TL_ERROR_INTERNAL);
    }
    rpc_query = std::move(it_rpc_query->second);
    component_query = std::move(it_component_query->second);
  }

  if (rpc_query.is_null()) {
    co_return make_fetch_error(string{"can't use rpc_tl_query_result for non-TL query"}, TL_ERROR_INTERNAL);
  }
  if (!rpc_query.get()->result_fetcher || rpc_query.get()->result_fetcher->empty()) {
    co_return make_fetch_error(string{"rpc query has empty result fetcher"}, TL_ERROR_INTERNAL);
  }
  if (rpc_query.get()->result_fetcher->is_typed) {
    co_return make_fetch_error(string{"can't get untyped result from typed TL query. Use consistent API for that"}, TL_ERROR_INTERNAL);
  }

  const auto data{co_await f$component_client_get_result(component_query)};

  // TODO: subscribe to rpc response event?
  // update rpc response extra info
  if (const auto it_response_extra_info{rpc_ctx.rpc_responses_extra_info.find(query_id)}; it_response_extra_info != rpc_ctx.rpc_responses_extra_info.end()) {
    const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
    it_response_extra_info->second.second = {data.size(), timestamp - std::get<1>(it_response_extra_info->second.second)};
    it_response_extra_info->second.first = rpc_response_extra_info_status_t::READY;
  }

  rpc_ctx.rpc_buffer.clean();
  rpc_ctx.rpc_buffer.store_bytes(data.c_str(), data.size());

  co_return fetch_function_untyped(rpc_query);
}

task_t<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_one_impl(int64_t query_id, const RpcErrorFactory &error_factory) noexcept {
  if (query_id < RPC_VALID_QUERY_ID_RANGE_START) {
    co_return error_factory.make_error(string{"wrong query_id"}, TL_ERROR_WRONG_QUERY_ID);
  }

  auto &rpc_ctx{RpcComponentContext::get()};
  class_instance<RpcTlQuery> rpc_query{};
  class_instance<C$ComponentQuery> component_query{};

  {
    const auto it_rpc_query{rpc_ctx.pending_rpc_queries.find(query_id)};
    const auto it_component_query{rpc_ctx.pending_component_queries.find(query_id)};

    vk::final_action finalizer{[&rpc_ctx, it_rpc_query, it_component_query]() {
      rpc_ctx.pending_rpc_queries.erase(it_rpc_query);
      rpc_ctx.pending_component_queries.erase(it_component_query);
    }};

    if (it_rpc_query == rpc_ctx.pending_rpc_queries.end() || it_component_query == rpc_ctx.pending_component_queries.end()) {
      co_return error_factory.make_error(string{"unexpectedly could not find query in pending queries"}, TL_ERROR_INTERNAL);
    }
    rpc_query = std::move(it_rpc_query->second);
    component_query = std::move(it_component_query->second);
  }

  if (rpc_query.is_null()) {
    co_return error_factory.make_error(string{"can't use rpc_tl_query_result for non-TL query"}, TL_ERROR_INTERNAL);
  }
  if (!rpc_query.get()->result_fetcher || rpc_query.get()->result_fetcher->empty()) {
    co_return error_factory.make_error(string{"rpc query has empty result fetcher"}, TL_ERROR_INTERNAL);
  }
  if (!rpc_query.get()->result_fetcher->is_typed) {
    co_return error_factory.make_error(string{"can't get typed result from untyped TL query. Use consistent API for that"}, TL_ERROR_INTERNAL);
  }

  const auto data{co_await f$component_client_get_result(component_query)};

  // TODO: subscribe to rpc response event?
  // update rpc response extra info
  if (const auto it_response_extra_info{rpc_ctx.rpc_responses_extra_info.find(query_id)}; it_response_extra_info != rpc_ctx.rpc_responses_extra_info.end()) {
    const auto timestamp{std::chrono::duration<double>{std::chrono::system_clock::now().time_since_epoch()}.count()};
    it_response_extra_info->second.second = {data.size(), timestamp - std::get<1>(it_response_extra_info->second.second)};
    it_response_extra_info->second.first = rpc_response_extra_info_status_t::READY;
  }

  rpc_ctx.rpc_buffer.clean();
  rpc_ctx.rpc_buffer.store_bytes(data.c_str(), data.size());

  co_return fetch_function_typed(rpc_query, error_factory);
}

} // namespace rpc_impl_

// === Rpc Store ==================================================================================

bool f$store_int(int64_t v) noexcept {
  if (unlikely(is_int32_overflow(v))) {
    php_warning("Got int32 overflow on storing '%" PRIi64 "', the value will be casted to '%d'", v, static_cast<int32_t>(v));
  }
  RpcComponentContext::get().rpc_buffer.store_i32(v);
  return true;
}

bool f$store_long(int64_t v) noexcept {
  RpcComponentContext::get().rpc_buffer.store_i64(v);
  return true;
}

bool f$store_float(double v) noexcept {
  RpcComponentContext::get().rpc_buffer.store_f32(v);
  return true;
}

bool f$store_double(double v) noexcept {
  RpcComponentContext::get().rpc_buffer.store_f64(v);
  return true;
}

bool f$store_string(const string &v) noexcept {
  RpcComponentContext::get().rpc_buffer.store_string(v.c_str(), v.size());
  return true;
}

// === Rpc Fetch ==================================================================================

int64_t f$fetch_int() noexcept {
  return static_cast<int64_t>(RpcComponentContext::get().rpc_buffer.fetch_i32().value_or(0));
}

int64_t f$fetch_long() noexcept {
  return RpcComponentContext::get().rpc_buffer.fetch_i64().value_or(0);
}

double f$fetch_double() noexcept {
  return RpcComponentContext::get().rpc_buffer.fetch_f64().value_or(0.0);
}

double f$fetch_float() noexcept {
  return static_cast<double>(RpcComponentContext::get().rpc_buffer.fetch_f32().value_or(0));
}

string f$fetch_string() noexcept {
  const auto [str_buf, str_len] = RpcComponentContext::get().rpc_buffer.fetch_string();
  return string(str_buf, str_len);
}

// === Rpc Query ==================================================================================

task_t<array<int64_t>> f$rpc_tl_query(string actor, array<mixed> tl_objects, double timeout, bool ignore_answer,
                                      class_instance<C$KphpRpcRequestsExtraInfo> requests_extra_info, bool need_responses_extra_info) noexcept {
  if (ignore_answer && need_responses_extra_info) {
    php_warning("Both $ignore_answer and $need_responses_extra_info are 'true'. Can't collect metrics for ignored answers");
  }

  bool collect_resp_extra_info = !ignore_answer && need_responses_extra_info;
  array<int64_t> query_ids{tl_objects.size()};
  array<rpc_request_extra_info_t> req_extra_info_arr{tl_objects.size()};

  for (const auto &it : tl_objects) {
    const auto query_info{co_await rpc_impl_::rpc_tl_query_one_impl(actor, it.get_value(), timeout, collect_resp_extra_info, ignore_answer)};
    query_ids.set_value(it.get_key(), query_info.id);
    req_extra_info_arr.set_value(it.get_key(), rpc_request_extra_info_t{query_info.request_size});
  }

  if (!requests_extra_info.is_null()) {
    requests_extra_info->extra_info_arr = std::move(req_extra_info_arr);
  }
  co_return query_ids;
}

task_t<array<array<mixed>>> f$rpc_tl_query_result(array<int64_t> query_ids) noexcept {
  array<array<mixed>> res{query_ids.size()};
  for (const auto &it : query_ids) {
    res.set_value(it.get_key(), co_await rpc_impl_::rpc_tl_query_result_one_impl(it.get_value()));
  }
  co_return res;
}

// === Rpc Misc ==================================================================================

void f$rpc_clean() noexcept {
  RpcComponentContext::get().rpc_buffer.clean();
}

// === Misc =======================================================================================

bool is_int32_overflow(int64_t v) noexcept {
  // f$store_int function is used for int and 'magic' storing,
  // 'magic' can be assigned via hex literals which may set the 32nd bit,
  // this is why we additionally check for the uint32_t here
  const auto v32 = static_cast<int32_t>(v);
  return vk::none_of_equal(v, int64_t{v32}, int64_t{static_cast<uint32_t>(v32)});
}

void store_raw_vector_double(const array<double> &vector) noexcept { // TODO: didn't we forget vector's length?
  RpcComponentContext::get().rpc_buffer.store_bytes(reinterpret_cast<const char *>(vector.get_const_vector_pointer()), sizeof(double) * vector.count());
}

void fetch_raw_vector_double(array<double> &vector, int64_t num_elems) noexcept {
  auto &rpc_buf{RpcComponentContext::get().rpc_buffer};
  const auto len_bytes{sizeof(double) * num_elems};
  if (rpc_buf.remaining() < len_bytes) {
    return; // TODO: error handling
  }
  vector.memcpy_vector(num_elems, rpc_buf.data() + rpc_buf.pos());
  rpc_buf.adjust(len_bytes);
}
