// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <tuple>
#include <utility>

#include "common/rpc-error-codes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"

namespace kphp::rpc {

std::expected<query_handle, int32_t> send_and_get_handle(std::string_view actor, bool collect_responses_extra_info, bool ignore_answer,
                                                         std::chrono::milliseconds timeout, double timestamp, int64_t query_id,
                                                         std::span<const std::byte> request_buffer) noexcept {
  auto& rpc_client_instance_st{RpcClientInstanceState::get()};
  auto& string_lib_ctx{StringLibContext::get()};

  if (const auto& [opt_new_extra_header, cur_extra_header_size]{kphp::rpc::regularize_extra_headers(request_buffer, ignore_answer)}; opt_new_extra_header) {
    std::span<const std::byte> new_header{reinterpret_cast<const std::byte*>(std::addressof(*opt_new_extra_header)),
                                          sizeof(std::remove_cvref_t<decltype(*opt_new_extra_header)>)};
    std::span<const std::byte> request_body{request_buffer.subspan(cur_extra_header_size)};

    std::span<std::byte> new_header_and_request{reinterpret_cast<std::byte*>(string_lib_ctx.static_buf.get()), new_header.size() + request_body.size()};

    std::ranges::copy(new_header, new_header_and_request.subspan(0, new_header.size()).begin());
    std::ranges::copy(request_body, new_header_and_request.subspan(new_header.size()).begin());

    request_buffer = new_header_and_request;
  }

  auto rpc_d_exp{k2::rpc_send_request(actor, request_buffer)};
  if (!rpc_d_exp) {
    return std::unexpected{rpc_d_exp.error()};
  }
  k2::descriptor rpc_d{*rpc_d_exp};

  // create response extra info
  if (collect_responses_extra_info) {
    rpc_client_instance_st.rpc_responses_extra_info.emplace(query_id, std::make_pair(response_extra_info_status::not_ready, response_extra_info{0, timestamp}));
  }

  std::chrono::nanoseconds deadline{timeout_to_deadline(timeout)};

  return {query_handle{rpc_d, query_id, deadline, collect_responses_extra_info}};
}

std::expected<string, std::pair<int32_t, string>> query_handle::get_ready_response() noexcept {
  std::expected<size_t, int32_t> first_response_size{k2::rpc_get_response_size(rpc_d)};
  if (!first_response_size) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
  }
  // TODO remove allocation
  string response{reinterpret_cast<char*>(k2::alloc(*first_response_size)), static_cast<string::size_type>(*first_response_size)};
  std::expected<void, int32_t> response_fetch_result{k2::rpc_fetch_response(rpc_d, {reinterpret_cast<std::byte*>(response.buffer()), response.size()})};
  if (!response_fetch_result) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
  }

  // update response extra info if needed
  if (collect_responses_extra_info) {
    auto& rpc_client_instance_st{RpcClientInstanceState::get()};
    auto& extra_info_map{rpc_client_instance_st.rpc_responses_extra_info};
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

} // namespace kphp::rpc
