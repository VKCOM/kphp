// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/rpc/init-functions.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr std::string_view RPC_REQUEST_ID = "RPC_REQUEST_ID";
constexpr std::string_view RPC_ACTOR_ID = "RPC_ACTOR_ID";
constexpr std::string_view RPC_REMOTE_IP = "RPC_REMOTE_IP";
constexpr std::string_view RPC_REMOTE_PORT = "RPC_REMOTE_PORT";
constexpr std::string_view RPC_REMOTE_PID = "RPC_REMOTE_PID";
constexpr std::string_view RPC_REMOTE_UTIME = "RPC_REMOTE_UTIME";

constexpr std::string_view RPC_EXTRA_FLAGS = "RPC_EXTRA_FLAGS";
constexpr std::string_view RPC_EXTRA_WAIT_BINLOG_POS = "RPC_EXTRA_WAIT_BINLOG_POS";
constexpr std::string_view RPC_EXTRA_STRING_FORWARD_KEYS = "RPC_EXTRA_STRING_FORWARD_KEYS";
constexpr std::string_view RPC_EXTRA_INT_FORWARD_KEYS = "RPC_EXTRA_INT_FORWARD_KEYS";
constexpr std::string_view RPC_EXTRA_STRING_FORWARD = "RPC_EXTRA_STRING_FORWARD";
constexpr std::string_view RPC_EXTRA_INT_FORWARD = "RPC_EXTRA_INT_FORWARD";
constexpr std::string_view RPC_EXTRA_CUSTOM_TIMEOUT_MS = "RPC_EXTRA_CUSTOM_TIMEOUT_MS";
constexpr std::string_view RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION = "RPC_EXTRA_SUPPORTED_COMPRESSION_VERSION";
constexpr std::string_view RPC_EXTRA_RANDOM_DELAY = "RPC_EXTRA_RANDOM_DELAY";
constexpr std::string_view RPC_EXTRA_PERSISTENT_QUERY = "RPC_EXTRA_PERSISTENT_QUERY";
constexpr std::string_view RPC_EXTRA_TRACE_CONTEXT = "RPC_EXTRA_TRACE_CONTEXT";
constexpr std::string_view RPC_EXTRA_EXECUTION_CONTEXT = "RPC_EXTRA_EXECUTION_CONTEXT";

void process_rpc_invoke_req_extra(const tl::rpcInvokeReqExtra& extra, PhpScriptBuiltInSuperGlobals& superglobals) noexcept {
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
  if (extra.opt_persistent_query) {
    const auto& persistent_query{*extra.opt_persistent_query};
    std::visit(
        [&superglobals](const auto& value) noexcept {
          using value_t = std::remove_cvref_t<decltype(value)>;

          constexpr std::string_view persistent_query_uuid_key_name{"persistent_query_uuid"};
          constexpr std::string_view persistent_slot_uuid_key_name{"persistent_slot_uuid"};
          constexpr std::string_view lo_key_name{"lo"};
          constexpr std::string_view hi_key_name{"hi"};

          if constexpr (std::is_same_v<value_t, tl::exactlyOnce::prepareRequest>) {
            array<mixed> persistent_query_uuid{array_size{2, false}};
            persistent_query_uuid.emplace_value(string{lo_key_name.data(), static_cast<string::size_type>(lo_key_name.size())},
                                                static_cast<int64_t>(value.persistent_query_uuid.lo.value));
            persistent_query_uuid.emplace_value(string{hi_key_name.data(), static_cast<string::size_type>(hi_key_name.size())},
                                                static_cast<int64_t>(value.persistent_query_uuid.hi.value));

            array<mixed> out{array_size{1, false}};
            out.emplace_value(string{persistent_query_uuid_key_name.data(), static_cast<string::size_type>(persistent_query_uuid_key_name.size())},
                              std::move(persistent_query_uuid));

            superglobals.v$_SERVER.set_value(string{RPC_EXTRA_PERSISTENT_QUERY.data(), RPC_EXTRA_PERSISTENT_QUERY.size()}, std::move(out));
          } else if constexpr (std::is_same_v<value_t, tl::exactlyOnce::commitRequest>) {
            array<mixed> persistent_query_uuid{array_size{2, false}};
            persistent_query_uuid.emplace_value(string{lo_key_name.data(), static_cast<string::size_type>(lo_key_name.size())},
                                                static_cast<int64_t>(value.persistent_query_uuid.lo.value));
            persistent_query_uuid.emplace_value(string{hi_key_name.data(), static_cast<string::size_type>(hi_key_name.size())},
                                                static_cast<int64_t>(value.persistent_query_uuid.hi.value));

            array<mixed> persistent_slot_uuid{array_size{2, false}};
            persistent_slot_uuid.emplace_value(string{lo_key_name.data(), static_cast<string::size_type>(lo_key_name.size())},
                                               static_cast<int64_t>(value.persistent_slot_uuid.lo.value));
            persistent_slot_uuid.emplace_value(string{hi_key_name.data(), static_cast<string::size_type>(hi_key_name.size())},
                                               static_cast<int64_t>(value.persistent_slot_uuid.hi.value));

            array<mixed> out{array_size{2, false}};
            out.emplace_value(string{persistent_query_uuid_key_name.data(), static_cast<string::size_type>(persistent_query_uuid_key_name.size())},
                              std::move(persistent_query_uuid));
            out.emplace_value(string{persistent_slot_uuid_key_name.data(), static_cast<string::size_type>(persistent_slot_uuid_key_name.size())},
                              std::move(persistent_slot_uuid));

            superglobals.v$_SERVER.set_value(string{RPC_EXTRA_PERSISTENT_QUERY.data(), RPC_EXTRA_PERSISTENT_QUERY.size()}, std::move(out));
          } else {
            static_assert(false, "exactlyOnce::PersistentRequest only supports prepareRequest and commitRequest");
          }
        },
        persistent_query.request);
  }
  if (extra.opt_trace_context) {
    const auto& trace_context{*extra.opt_trace_context};

    constexpr std::string_view fields_mask_key_name{"fields_mask"};
    constexpr std::string_view trace_id_key_name{"trace_id"};
    constexpr std::string_view lo_key_name{"lo"};
    constexpr std::string_view hi_key_name{"hi"};
    constexpr std::string_view parent_id_key_name{"parent_id"};
    constexpr std::string_view source_id_key_name{"source_id"};

    // + 2 for fields_mask and trace_id, + 1 if there is a parent_id, + 1 if there is a source_id
    const int64_t out_size{2 + static_cast<int64_t>(trace_context.opt_parent_id.has_value()) + static_cast<int64_t>(trace_context.opt_source_id.has_value())};

    array<mixed> trace_id{array_size{2, false}};
    trace_id.emplace_value(string{lo_key_name.data(), static_cast<string::size_type>(lo_key_name.size())},
                           static_cast<int64_t>(trace_context.trace_id.lo.value));
    trace_id.emplace_value(string{hi_key_name.data(), static_cast<string::size_type>(hi_key_name.size())},
                           static_cast<int64_t>(trace_context.trace_id.hi.value));

    array<mixed> out{array_size{out_size, false}};
    out.emplace_value(string{fields_mask_key_name.data(), static_cast<string::size_type>(fields_mask_key_name.size())},
                      static_cast<int64_t>(trace_context.get_flags().value));
    out.emplace_value(string{trace_id_key_name.data(), static_cast<string::size_type>(trace_id_key_name.size())}, std::move(trace_id));

    if (trace_context.opt_parent_id) {
      out.emplace_value(string{parent_id_key_name.data(), static_cast<string::size_type>(parent_id_key_name.size())},
                        static_cast<int64_t>(trace_context.opt_parent_id->value));
    }
    if (trace_context.opt_source_id) {
      const std::string_view& opt_source_id_value{trace_context.opt_source_id->value};
      out.emplace_value(string{source_id_key_name.data(), static_cast<string::size_type>(source_id_key_name.size())},
                        string{opt_source_id_value.data(), static_cast<string::size_type>(opt_source_id_value.size())});
    }

    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_TRACE_CONTEXT.data(), RPC_EXTRA_TRACE_CONTEXT.size()}, std::move(out));
  }
  if (extra.opt_execution_context) {
    const std::string_view& execution_context{extra.opt_execution_context->value};
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_EXECUTION_CONTEXT.data(), RPC_EXTRA_EXECUTION_CONTEXT.size()},
                                     string{execution_context.data(), static_cast<string::size_type>(execution_context.size())});
  }
}

} // namespace

namespace kphp::rpc {

void init_server(kphp::component::stream&& request_stream, kphp::stl::vector<std::byte, kphp::memory::script_allocator>&& request) noexcept {
  tl::fetcher tlf{request};
  tl::K2InvokeRpc invoke_rpc{};
  if (!invoke_rpc.fetch(tlf)) [[unlikely]] {
    kphp::log::error("erroneous rpc request");
  }

  auto& rpc_server_instance_st{RpcServerInstanceState::get()};
  rpc_server_instance_st.request_stream = std::move(request_stream);
  rpc_server_instance_st.query_id = invoke_rpc.query_id.value;
  rpc_server_instance_st.tl_storer.store_bytes(invoke_rpc.query);
  rpc_server_instance_st.tl_fetcher = tl::fetcher{rpc_server_instance_st.tl_storer.view()};

  tl::magic request_magic{};
  if (tl::fetcher tlf{rpc_server_instance_st.tl_storer.view()}; !request_magic.fetch(tlf)) [[unlikely]] {
    kphp::log::error("erroneous rpc request");
  }

  auto& superglobals{PhpScriptMutableGlobals::current().get_superglobals()};
  superglobals.v$_SERVER.set_value(string{RPC_REQUEST_ID.data(), RPC_REQUEST_ID.size()}, invoke_rpc.query_id.value);
  superglobals.v$_SERVER.set_value(string{RPC_REMOTE_IP.data(), RPC_REMOTE_IP.size()}, static_cast<int64_t>(invoke_rpc.net_pid.get_ip()));
  superglobals.v$_SERVER.set_value(string{RPC_REMOTE_PORT.data(), RPC_REMOTE_PORT.size()}, static_cast<int64_t>(invoke_rpc.net_pid.get_port()));
  superglobals.v$_SERVER.set_value(string{RPC_REMOTE_PID.data(), RPC_REMOTE_PID.size()}, static_cast<int64_t>(invoke_rpc.net_pid.get_pid()));
  superglobals.v$_SERVER.set_value(string{RPC_REMOTE_UTIME.data(), RPC_REMOTE_UTIME.size()}, static_cast<int64_t>(invoke_rpc.net_pid.get_utime()));
  if (invoke_rpc.opt_actor_id) {
    superglobals.v$_SERVER.set_value(string{RPC_ACTOR_ID.data(), RPC_ACTOR_ID.size()}, (*invoke_rpc.opt_actor_id).value);
  }

  const std::optional<tl::mask> opt_extra_fields_mask{invoke_rpc.opt_extra.transform([](const auto& extra) noexcept { return extra.get_flags(); })};
  if (invoke_rpc.opt_extra) {
    kphp::log::assertion(opt_extra_fields_mask.has_value());
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_FLAGS.data(), RPC_EXTRA_FLAGS.size()}, static_cast<int64_t>((*opt_extra_fields_mask).value));
    process_rpc_invoke_req_extra(*invoke_rpc.opt_extra, superglobals);
  }
  kphp::log::info("rpc server initialized with: "
                  "remote pid -> {}, "
                  "remote port -> {}, "
                  "query_id -> {}, "
                  "actor_id -> {}, "
                  "extra -> {:#b}, "
                  "request -> {:#x}",
                  invoke_rpc.net_pid.get_pid(), invoke_rpc.net_pid.get_port(), invoke_rpc.query_id.value,
                  invoke_rpc.opt_actor_id.has_value() ? (*invoke_rpc.opt_actor_id).value : 0,
                  opt_extra_fields_mask.has_value() ? (*opt_extra_fields_mask).value : 0, request_magic.value);
}

} // namespace kphp::rpc
