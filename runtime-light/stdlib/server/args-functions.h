// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/component-state.h"

inline Optional<string> f$ini_get(const string& key) noexcept {
  const auto& component_st{ComponentState::get()};
  return component_st.ini_opts.has_key(key) ? Optional<string>{component_st.ini_opts.get_value(key)} : Optional<string>{false};
}

inline mixed f$kphp_get_runtime_config() noexcept {
  const auto& component_st{ComponentState::get()};
  return component_st.runtime_config;
}
