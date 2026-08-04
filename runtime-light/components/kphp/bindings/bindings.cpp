// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/components/kphp/state/component-state.h"
#include "runtime-light/components/kphp/state/image-state.h"
#include "runtime-light/components/kphp/state/instance-state.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

const AllocatorState& AllocatorState::get() noexcept {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_allocator_state;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return component_state_ptr->component_allocator_state;
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) {
    return image_state_ptr->image_allocator_state;
  }
  kphp::log::error("can't find allocator state");
}

RuntimeContext& RuntimeContext::get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->runtime_context;
  }
  kphp::log::error("unexpected access to RuntimeContext");
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

ErrorHandlingState& ErrorHandlingState::get() noexcept {
  return InstanceState::get().error_handling_instance_state;
}

std::optional<std::reference_wrapper<ErrorHandlingState>> ErrorHandlingState::try_get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->error_handling_instance_state;
  }
  return std::nullopt;
}

namespace kphp::coro {

instance_state& instance_state::get() noexcept {
  return InstanceState::get().coroutine_instance_state;
}

io_scheduler& io_scheduler::get() noexcept {
  return InstanceState::get().io_scheduler;
}

} // namespace kphp::coro

namespace kphp::log {

std::optional<std::reference_wrapper<contextual_tags>> contextual_tags::try_get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_tags;
  }
  return std::nullopt;
}

} // namespace kphp::log
