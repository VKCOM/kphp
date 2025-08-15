//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/error-handling-state.h"

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/instance-state.h"

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
