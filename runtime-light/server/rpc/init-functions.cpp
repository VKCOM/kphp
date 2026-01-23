// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/rpc/init-functions.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
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
    std::visit(
        [&superglobals](const auto& value) noexcept {
          using value_t = std::remove_cvref_t<decltype(value)>;

          constexpr std::string_view exactlyOnce_prepareRequest_sv = "exactlyOnce.prepareRequest";
          constexpr std::string_view persistent_query_uuid_sv = "persistent_query_uuid";
          constexpr std::string_view exactlyOnce_commitRequest_sv = "exactlyOnce.commitRequest";
          constexpr std::string_view persistent_slot_uuid_sv = "persistent_slot_uuid";
          constexpr std::string_view exactlyOnce_uuid_sv = "exactlyOnce.uuid";
          constexpr std::string_view lo_sv = "lo";
          constexpr std::string_view hi_sv = "hi";

          const auto underline = string{"_", 1};

          array out{std::pair{underline, mixed{}},
                    std::pair{string{persistent_query_uuid_sv.data(), static_cast<string::size_type>(persistent_query_uuid_sv.size())}, mixed{}}};

          if constexpr (std::is_same_v<value_t, tl::exactlyOnce::prepareRequest>) {
            out.emplace_value(underline,
                              mixed{string{exactlyOnce_prepareRequest_sv.data(), static_cast<string::size_type>(exactlyOnce_prepareRequest_sv.size())}});

            out.emplace_value(
                string{persistent_query_uuid_sv.data(), static_cast<string::size_type>(persistent_query_uuid_sv.size())},
                mixed{array{std::pair{underline, mixed{string{exactlyOnce_uuid_sv.data(), static_cast<string::size_type>(exactlyOnce_uuid_sv.size())}}},
                            std::pair{string{lo_sv.data(), static_cast<string::size_type>(lo_sv.size())}, mixed{value.persistent_query_uuid.lo.value}},
                            std::pair{string{hi_sv.data(), static_cast<string::size_type>(hi_sv.size())}, mixed{value.persistent_query_uuid.hi.value}}}});

          } else if constexpr (std::is_same_v<value_t, tl::exactlyOnce::commitRequest>) {
            out.emplace_value(underline,
                              mixed{string{exactlyOnce_commitRequest_sv.data(), static_cast<string::size_type>(exactlyOnce_commitRequest_sv.size())}});

            out.emplace_value(
                string{persistent_query_uuid_sv.data(), static_cast<string::size_type>(persistent_query_uuid_sv.size())},
                mixed{array{std::pair{underline, mixed{string{exactlyOnce_uuid_sv.data(), static_cast<string::size_type>(exactlyOnce_uuid_sv.size())}}},
                            std::pair{string{lo_sv.data(), static_cast<string::size_type>(lo_sv.size())}, mixed{value.persistent_query_uuid.lo.value}},
                            std::pair{string{hi_sv.data(), static_cast<string::size_type>(hi_sv.size())}, mixed{value.persistent_query_uuid.hi.value}}}});

            out.set_value(
                string{persistent_slot_uuid_sv.data(), static_cast<string::size_type>(persistent_slot_uuid_sv.size())},
                mixed{array{std::pair{underline, mixed{string{exactlyOnce_uuid_sv.data(), static_cast<string::size_type>(exactlyOnce_uuid_sv.size())}}},
                            std::pair{string{lo_sv.data(), static_cast<string::size_type>(lo_sv.size())}, mixed{value.persistent_slot_uuid.lo.value}},
                            std::pair{string{hi_sv.data(), static_cast<string::size_type>(hi_sv.size())}, mixed{value.persistent_slot_uuid.hi.value}}}});
          } else {
            static_assert(false, "exactlyOnce::PersistentRequest only supports prepareRequest and commitRequest");
          }

          superglobals.v$_SERVER.set_value(string{RPC_EXTRA_PERSISTENT_QUERY.data(), RPC_EXTRA_PERSISTENT_QUERY.size()}, std::move(out));
        },
        extra.opt_persistent_query->request);
  }
  if (extra.opt_trace_context) {
    const auto& trace_context{*extra.opt_trace_context};

    constexpr std::string_view tracing_traceContext_sv = "tracing.traceContext";
    constexpr std::string_view fields_mask_sv = "fields_mask";
    constexpr std::string_view trace_id_sv = "trace_id";
    constexpr std::string_view tracing_TraceID_sv = "tracing.TraceID";
    constexpr std::string_view lo_sv = "lo";
    constexpr std::string_view hi_sv = "hi";
    constexpr std::string_view parent_id_sv = "parent_id";
    constexpr std::string_view source_id_sv = "source_id";
    constexpr std::string_view reserved_status_0_sv = "reserved_status_0";
    constexpr std::string_view reserved_status_1_sv = "reserved_status_1";
    constexpr std::string_view reserved_level_0_sv = "reserved_level_0";
    constexpr std::string_view reserved_level_1_sv = "reserved_level_1";
    constexpr std::string_view reserved_level_2_sv = "reserved_level_2";
    constexpr std::string_view debug_flag_sv = "debug_flag";

    array out{
        std::pair{string{"_", 1}, mixed{string{tracing_traceContext_sv.data(), static_cast<string::size_type>(tracing_traceContext_sv.size())}}},
        std::pair{string{fields_mask_sv.data(), static_cast<string::size_type>(fields_mask_sv.size())}, mixed{trace_context.fields_mask.value}},
        std::pair{string{trace_id_sv.data(), static_cast<string::size_type>(trace_id_sv.size())},
                  mixed{array{std::pair{string{"_", 1}, mixed{string{tracing_TraceID_sv.data(), static_cast<string::size_type>(tracing_TraceID_sv.size())}}},
                              std::pair{string{lo_sv.data(), static_cast<string::size_type>(lo_sv.size())}, mixed{trace_context.trace_id.lo.value}},
                              std::pair{string{hi_sv.data(), static_cast<string::size_type>(hi_sv.size())}, mixed{trace_context.trace_id.hi.value}}}}}};

    if (trace_context.opt_parent_id) {
      out.set_value(string{parent_id_sv.data(), static_cast<string::size_type>(parent_id_sv.size())}, trace_context.opt_parent_id->value);
    }
    if (trace_context.opt_source_id) {
      const std::string& opt_source_id_value{trace_context.opt_source_id->value};
      out.set_value(string{source_id_sv.data(), static_cast<string::size_type>(source_id_sv.size())},
                    string{opt_source_id_value.data(), static_cast<string::size_type>(opt_source_id_value.size())});
    }
    if (trace_context.reserved_status_0) {
      out.set_value(string{reserved_status_0_sv.data(), static_cast<string::size_type>(reserved_status_0_sv.size())}, true);
    }
    if (trace_context.reserved_status_1) {
      out.set_value(string{reserved_status_1_sv.data(), static_cast<string::size_type>(reserved_status_1_sv.size())}, true);
    }
    if (trace_context.reserved_level_0) {
      out.set_value(string{reserved_level_0_sv.data(), static_cast<string::size_type>(reserved_level_0_sv.size())}, true);
    }
    if (trace_context.reserved_level_1) {
      out.set_value(string{reserved_level_1_sv.data(), static_cast<string::size_type>(reserved_level_1_sv.size())}, true);
    }
    if (trace_context.reserved_level_2) {
      out.set_value(string{reserved_level_2_sv.data(), static_cast<string::size_type>(reserved_level_2_sv.size())}, true);
    }
    if (trace_context.debug_flag) {
      out.set_value(string{debug_flag_sv.data(), static_cast<string::size_type>(debug_flag_sv.size())}, true);
    }

    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_TRACE_CONTEXT.data(), RPC_EXTRA_TRACE_CONTEXT.size()}, std::move(out));
  }
  if (extra.opt_execution_context) {
    const auto& execution_context{extra.opt_execution_context->value};
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
  if (invoke_rpc.opt_extra) {
    superglobals.v$_SERVER.set_value(string{RPC_EXTRA_FLAGS.data(), RPC_EXTRA_FLAGS.size()}, static_cast<int64_t>((*invoke_rpc.opt_extra).flags.value));
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
                  invoke_rpc.opt_extra.has_value() ? (*invoke_rpc.opt_extra).flags.value : 0, request_magic.value);
}

} // namespace kphp::rpc
