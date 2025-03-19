// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/component-state.h"

inline Optional<string> f$ini_get(const string &key) noexcept {
  const auto &component_st{ComponentState::get()};
  return component_st.ini_opts.has_key(key) ? Optional<string>{component_st.ini_opts.get_value(key)} : Optional<string>{false};
}

Optional<array<mixed>> f$getopt(const string &short_options, const array<string> &long_options = {},
                                Optional<std::optional<std::reference_wrapper<string>>> rest_index = {}) noexcept;
