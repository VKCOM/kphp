// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/components/confdata/state/component-state.h"
#include "runtime-light/components/confdata/state/instance-state.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

auto instance_state::get() noexcept -> instance_state& {
  return InstanceState::get().m_coroutine_instance_state;
}

auto io_scheduler::get() noexcept -> io_scheduler& {
  return InstanceState::get().m_io_scheduler;
}

} // namespace kphp::coro

namespace kphp::log {

auto contextual_tags::try_get() noexcept -> std::optional<std::reference_wrapper<contextual_tags>> {
  return std::nullopt;
}

} // namespace kphp::log

auto AllocatorState::get() noexcept -> const AllocatorState& {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->m_allocator_state;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return component_state_ptr->m_allocator_state;
  }
  kphp::log::error("can't find allocator state");
}

auto RuntimeCoroutineAllocator::get() noexcept -> RuntimeCoroutineAllocator& {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->coroutine_allocator;
  }
  kphp::log::error("can't find runtime coroutine allocator");
}

auto ErrorHandlingState::try_get() noexcept -> std::optional<std::reference_wrapper<ErrorHandlingState>> {
  return std::nullopt; // confdata doesn't support PHP error handling
}

auto RuntimeContext::get() noexcept -> RuntimeContext& {
  kphp::log::error("unexpected access to RuntimeContext"); // confdata doesn't have a runtime context
}
