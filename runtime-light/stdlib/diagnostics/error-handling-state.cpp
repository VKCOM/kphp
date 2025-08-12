//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/error-handling-state.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/instance-state.h"

ErrorHandlingState::ErrorHandlingState() noexcept {
  const auto& component_st{ComponentState::get()};
  if (const auto& default_level{component_st.ini_opts.get_value(string(INI_LEVEL_KEY.data(), INI_LEVEL_KEY.size()))};
      default_level.is_int() && static_cast<bool>(SUPPORTED_ERROR_LEVELS & default_level.to_int())) {
    minimum_log_level = default_level.to_int();
  }
}

ErrorHandlingState& ErrorHandlingState::get() noexcept {
  return InstanceState::get().error_handling_instance_state;
}
