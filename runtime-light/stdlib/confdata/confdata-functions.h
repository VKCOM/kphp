// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/confdata/confdata-state.h"

inline bool f$is_confdata_loaded() noexcept {
  return ConfdataInstanceState::get().is_initialized();
}

auto f$confdata_get_value(const string& key) noexcept -> mixed;

auto f$confdata_get_values_by_any_wildcard(const string& wildcard) noexcept -> array<mixed>;

auto f$confdata_get_values_by_predefined_wildcard(const string& wildcard) noexcept -> array<mixed>;
