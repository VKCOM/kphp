// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/allocator/runtime-coroutine-allocator.h"
#include "runtime-light/components/kphp/state/component-state.h"
#include "runtime-light/components/kphp/state/image-state.h"
#include "runtime-light/components/kphp/state/instance-state.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/cli/cli-instance-state.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/server/job-worker/job-worker-server-state.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/confdata/confdata-state.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/file-system-state.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/fork/wait-queue-state.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/job-worker/job-worker-client-state.h"
#include "runtime-light/stdlib/kml/kml-state.h"
#include "runtime-light/stdlib/math/math-state.h"
#include "runtime-light/stdlib/math/random-state.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-queue-state.h"
#include "runtime-light/stdlib/serialization/serialization-state.h"
#include "runtime-light/stdlib/string/regex-state.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/stdlib/system/system-state.h"
#include "runtime-light/stdlib/time/time-state.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"

namespace kphp::coro {

auto instance_state::get() noexcept -> instance_state& {
  return InstanceState::get().coroutine_instance_state;
}

auto io_scheduler::get() noexcept -> io_scheduler& {
  return InstanceState::get().io_scheduler;
}

} // namespace kphp::coro

namespace kphp::log {

auto contextual_tags::try_get() noexcept -> std::optional<std::reference_wrapper<contextual_tags>> {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_tags;
  }
  return std::nullopt;
}

} // namespace kphp::log

auto AllocatorState::get() noexcept -> const AllocatorState& {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_allocator_state;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return component_state_ptr->component_allocator_state;
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) {
    return image_state_ptr->image_allocator_state;
  }
  kphp::log::error("can't find allocator state");
}

auto RuntimeCoroutineAllocator::get() noexcept -> RuntimeCoroutineAllocator& {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->coroutine_allocator;
  }
  kphp::log::error("can't find runtime coroutine allocator");
}

auto RuntimeContext::get() noexcept -> RuntimeContext& {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->runtime_context;
  }
  kphp::log::error("unexpected access to RuntimeContext");
}

auto PhpScriptMutableGlobals::current() noexcept -> PhpScriptMutableGlobals& {
  return InstanceState::get().php_script_mutable_globals_singleton;
}

ErrorHandlingState::ErrorHandlingState() noexcept {
  const auto& component_st{ComponentState::get()};
  const auto& default_level_str{component_st.ini_opts.get_value(string{INI_ERROR_REPORTING_KEY.data(), INI_ERROR_REPORTING_KEY.size()})};
  if (default_level_str.empty() || !default_level_str.is_int()) {
    return;
  }

  const int64_t default_level{default_level_str.to_int()};
  minimum_log_level = static_cast<bool>(SUPPORTED_ERROR_LEVELS & default_level) ? default_level : minimum_log_level;
}

auto ErrorHandlingState::get() noexcept -> ErrorHandlingState& {
  return InstanceState::get().error_handling_instance_state;
}

auto ErrorHandlingState::try_get() noexcept -> std::optional<std::reference_wrapper<ErrorHandlingState>> {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->error_handling_instance_state;
  }
  return std::nullopt;
}

auto CLIInstanceInstance::get() noexcept -> CLIInstanceInstance& {
  return InstanceState::get().cli_instance_instate;
}

auto HttpServerInstanceState::get() noexcept -> HttpServerInstanceState& {
  return InstanceState::get().http_server_instance_state;
}

auto JobWorkerServerInstanceState::get() noexcept -> JobWorkerServerInstanceState& {
  return InstanceState::get().job_worker_server_instance_state;
}

auto RpcServerInstanceState::get() noexcept -> RpcServerInstanceState& {
  return InstanceState::get().rpc_server_instance_state;
}

auto ConfdataInstanceState::get() noexcept -> ConfdataInstanceState& {
  return InstanceState::get().confdata_instance_state;
}

auto CurlInstanceState::get() noexcept -> CurlInstanceState& {
  return InstanceState::get().curl_instance_state;
}

auto CurlImageState::get() noexcept -> const CurlImageState& {
  return ImageState::get().curl_image_state;
}

auto FileSystemImageState::get() noexcept -> const FileSystemImageState& {
  return ImageState::get().file_system_image_state;
}

auto ForkInstanceState::get() noexcept -> ForkInstanceState& {
  return InstanceState::get().fork_instance_state;
}

auto InstanceCacheInstanceState::get() noexcept -> InstanceCacheInstanceState& {
  return InstanceState::get().instance_cache_instance_state;
}

auto JobWorkerClientInstanceState::get() noexcept -> JobWorkerClientInstanceState& {
  return InstanceState::get().job_worker_client_instance_state;
}

template<>
auto KmlComponentState::get() noexcept -> const KmlComponentState& {
  return ComponentState::get().kml_component_state;
}

template<>
auto KmlComponentState::get_mutable() noexcept -> KmlComponentState& {
  return const_cast<KmlComponentState&>(KmlComponentState::get());
}

template<>
auto KmlInstanceState::get() noexcept -> KmlInstanceState& {
  return InstanceState::get().kml_instance_state;
}

auto MathInstanceState::get() noexcept -> MathInstanceState& {
  return InstanceState::get().math_instance_state;
}

auto MathImageState::get() noexcept -> const MathImageState& {
  return ImageState::get().math_image_state;
}

auto OutputInstanceState::get() noexcept -> OutputInstanceState& {
  return InstanceState::get().output_instance_state;
}

auto RandomInstanceState::get() noexcept -> RandomInstanceState& {
  return InstanceState::get().random_instance_state;
}

auto RegexInstanceState::get() noexcept -> RegexInstanceState& {
  return InstanceState::get().regex_instance_state;
}

auto RegexImageState::get() noexcept -> const RegexImageState& {
  return ImageState::get().regex_image_state;
}

auto RegexImageState::get_mutable() noexcept -> RegexImageState& {
  return ImageState::get_mutable().regex_image_state;
}

auto RpcClientInstanceState::get() noexcept -> RpcClientInstanceState& {
  return InstanceState::get().rpc_client_instance_state;
}

auto RpcImageState::get() noexcept -> const RpcImageState& {
  return ImageState::get().rpc_image_state;
}

auto RpcImageState::get_mutable() noexcept -> RpcImageState& {
  return ImageState::get_mutable().rpc_image_state;
}

auto RpcQueueInstanceState::get() noexcept -> RpcQueueInstanceState& {
  return InstanceState::get().rpc_queue_instance_state;
}

auto SerializationInstanceState::get() noexcept -> SerializationInstanceState& {
  return InstanceState::get().serialization_instance_state;
}

auto SerializationImageState::get() noexcept -> const SerializationImageState& {
  return ImageState::get().serialization_image_state;
}

auto StringInstanceState::get() noexcept -> StringInstanceState& {
  return InstanceState::get().string_instance_state;
}

auto StringImageState::get() noexcept -> const StringImageState& {
  return ImageState::get().string_image_state;
}

auto SystemInstanceState::get() noexcept -> SystemInstanceState& {
  return InstanceState::get().system_instance_state;
}

auto TimeInstanceState::get() noexcept -> TimeInstanceState& {
  return InstanceState::get().time_instance_state;
}

auto TimeImageState::get() noexcept -> const TimeImageState& {
  return ImageState::get().time_image_state;
}

auto TimeImageState::get_mutable() noexcept -> TimeImageState& {
  return ImageState::get_mutable().time_image_state;
}

auto WaitQueueInstanceState::get() noexcept -> WaitQueueInstanceState& {
  return InstanceState::get().wait_queue_instance_state;
}

auto WebInstanceState::get() noexcept -> WebInstanceState& {
  return InstanceState::get().web_instance_state;
}
