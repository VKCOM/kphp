// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/rpc/init-functions.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/rpc/rpc-server-instance-state.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr std::string_view RPC_REQUEST_ID = "RPC_REQUEST_ID";
constexpr std::string_view RPC_ACTOR_ID = "RPC_ACTOR_ID";
constexpr std::string_view RPC_EXTRA_FLAGS = "RPC_EXTRA_FLAGS";
constexpr std::string_view RPC_EXTRA_WAIT_BINLOG_POS = "RPC_EXTRA_WAIT_BINLOG_POS";
constexpr std::string_view RPC_EXTRA_STRING_FORWARD_KEYS = "RPC_EXTRA_STRING_FORWARD_KEYS";
constexpr std::string_view RPC_EXTRA_INT_FORWARD_KEYS = "RPC_EXTRA_INT_FORWARD_KEYS";
constexpr std::string_view RPC_EXTRA_STRING_FORWARD = "RPC_EXTRA_STRING_FORWARD";
constexpr std::string_view RPC_EXTRA_INT_FORWARD = "RPC_EXTRA_INT_FORWARD";
constexpr std::string_view RPC_EXTRA_CUSTOM_TIMEOUT_MS = "RPC_EXTRA_CUSTOM_TIMEOUT_MS";
constexpr std::string_view RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION = "RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION";
constexpr std::string_view RPC_EXTRA_RANDOM_DELAY = "RPC_EXTRA_RANDOM_DELAY";

void process_rpc_invoke_req_extra(const tl::rpcInvokeReqExtra& extra, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  superglobals.v$_SERVER.set_value(string{RPC_EXTRA_FLAGS.data(), RPC_EXTRA_FLAGS.size()}, static_cast<int64_t>(extra.flags));
  if (extra.opt_wait_binlog_pos.has_value()) {
    auto wait_binlog_pos{*extra.opt_wait_binlog_pos};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_WAIT_BINLOG_POS.data(), RPC_EXTRA_WAIT_BINLOG_POS.size()}, wait_binlog_pos.value);
  }
  if (extra.opt_string_forward_keys) {
    const auto& keys{*extra.opt_string_forward_keys};
    array<string> string_forward_keys{array_size{static_cast<int64_t>(keys.size()), true}};
    std::ranges::for_each(keys, [&string_forward_keys](const auto& key) noexcept {
      string_forward_keys.emplace_back(string{key.value.data(), static_cast<string::size_type>(key.value.size())});
    });
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_STRING_FORWARD_KEYS.data(), RPC_EXTRA_STRING_FORWARD_KEYS.size()}, std::move(string_forward_keys));
  }
  if (extra.opt_int_forward_keys) {
    const auto& keys{*extra.opt_int_forward_keys};
    array<int64_t> int_forward_keys{array_size{static_cast<int64_t>(keys.size()), true}};
    std::ranges::for_each(keys, [&int_forward_keys](const auto& key) noexcept { int_forward_keys.emplace_back(key.value); });
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_INT_FORWARD_KEYS.data(), RPC_EXTRA_INT_FORWARD_KEYS.size()}, std::move(int_forward_keys));
  }
  if (extra.opt_string_forward) {
    const auto& string_forward{*extra.opt_string_forward};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_STRING_FORWARD.data(), RPC_EXTRA_STRING_FORWARD.size()},
                                     string{string_forward.value.data(), static_cast<string::size_type>(string_forward.value.size())});
  }
  if (extra.opt_int_forward) {
    auto int_forward{*extra.opt_int_forward};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_INT_FORWARD.data(), RPC_EXTRA_INT_FORWARD.size()}, int_forward.value);
  }
  if (extra.opt_custom_timeout_ms) {
    auto custom_timeout_ms{*extra.opt_custom_timeout_ms};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_CUSTOM_TIMEOUT_MS.data(), RPC_EXTRA_CUSTOM_TIMEOUT_MS.size()},
                                     static_cast<int64_t>(custom_timeout_ms.value));
  }
  if (extra.opt_supported_compression_version) {
    auto supported_compression_version{*extra.opt_supported_compression_version};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION.data(), RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION.size()},
                                     static_cast<int64_t>(supported_compression_version.value));
  }
  if (extra.opt_random_delay) {
    auto random_delay{*extra.opt_random_delay};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_RANDOM_DELAY.data(), RPC_EXTRA_RANDOM_DELAY.size()}, random_delay.value);
  }
}

void process_dest_actor(const tl::rpcDestActor& dest_actor, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  superglobals.v$_SERVER.set_value(string{RPC_ACTOR_ID.data(), RPC_ACTOR_ID.size()}, dest_actor.actor_id.value);
  RpcServerInstanceState::get().buffer.store_bytes(dest_actor.query);
}

void process_dest_flags(const tl::rpcDestFlags& dest_flags, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  process_rpc_invoke_req_extra(dest_flags.extra, superglobals);
  RpcServerInstanceState::get().buffer.store_bytes(dest_flags.query);
}

void process_dest_actor_flags(const tl::rpcDestActorFlags& dest_actor_flags, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  superglobals.v$_SERVER.set_value(string{RPC_ACTOR_ID.data(), RPC_ACTOR_ID.size()}, dest_actor_flags.actor_id.value);
  process_rpc_invoke_req_extra(dest_actor_flags.extra, superglobals);
  RpcServerInstanceState::get().buffer.store_bytes(dest_actor_flags.query);
}

void process_no_headers(std::string_view query) noexcept {
  RpcServerInstanceState::get().buffer.store_bytes(query);
}

void process_rpc_invoke_req(const tl::rpcInvokeReq& rpc_invoke_req, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
  std::visit(
      [&superglobals](const auto& header) noexcept {
        using header_t = std::remove_cvref_t<decltype(header)>;

        if constexpr (std::same_as<header_t, tl::RpcDestActor>) {
          process_dest_actor(header.inner, superglobals);
        } else if constexpr (std::same_as<header_t, tl::RpcDestFlags>) {
          process_dest_flags(header.inner, superglobals);
        } else if constexpr (std::same_as<header_t, tl::RpcDestActorFlags>) {
          process_dest_actor_flags(header.inner, superglobals);
        } else if constexpr (std::same_as<header_t, std::string_view>) {
          process_no_headers(header);
        } else {
          static_assert(false, "non-exhaustive visitor!");
        }
      },
      rpc_invoke_req.query);
}

} // namespace

namespace kphp::rpc {

void init_server(tl::K2InvokeRpc invoke_rpc) noexcept {
  auto& rpc_invoke_req{invoke_rpc.rpc_invoke_req.inner};
  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  auto& superglobals{PhpScriptMutableGlobals::current().get_superglobals()};

  rpc_server_instance_st.query_id = rpc_invoke_req.query_id.value;

  // TODO: rpc remote info
  superglobals.v$_SERVER.set_value(string{RPC_REQUEST_ID.data(), RPC_REQUEST_ID.size()}, rpc_invoke_req.query_id.value);
  process_rpc_invoke_req(rpc_invoke_req, superglobals);
}

} // namespace kphp::rpc
