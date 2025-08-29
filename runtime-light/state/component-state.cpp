// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/component-state.h"

#include <cstring>
#include <iterator>
#include <string_view>
#include <utility>

#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

void ComponentState::parse_env() noexcept {
  for (auto i = 0; i < envc; ++i) {
    const auto [env_key, env_value]{k2::env_fetch(i)};

    string key_str{env_key.get()};
    key_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

    string value_str{env_value.get()};
    value_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

    env.set_value(key_str, value_str);
  }
  env.set_reference_counter_to(ExtraRefCnt::for_global_const);
}

void ComponentState::parse_ini_arg(std::string_view key_view, std::string_view value_view) noexcept {
  if (key_view.size() <= INI_ARG_PREFIX.size()) [[unlikely]] {
    kphp::log::warning("bad ini argument: {}", key_view);
    return;
  }
  string key_str{std::next(key_view.data(), INI_ARG_PREFIX.size()), static_cast<string::size_type>(key_view.size() - INI_ARG_PREFIX.size())};
  key_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

  string value_str{value_view.data(), static_cast<string::size_type>(value_view.size())};
  value_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

  ini_opts.set_value(key_str, value_str);
}

void ComponentState::parse_runtime_config_arg(std::string_view value_view) noexcept {
  if (auto opt_config{json_decode(value_view)}; opt_config) [[likely]] {
    runtime_config = *std::move(opt_config);
  } else {
    kphp::log::warning("runtime config isn't a valid JSON");
  }
}

void ComponentState::parse_args() noexcept {
  for (auto i = 0; i < argc; ++i) {
    const auto [arg_key, arg_value]{k2::arg_fetch(i)};
    const std::string_view key_view{arg_key.get(), std::strlen(arg_key.get())};
    const std::string_view value_view{arg_value.get(), std::strlen(arg_value.get())};

    if (key_view.starts_with(INI_ARG_PREFIX)) {
      parse_ini_arg(key_view, value_view);
    } else if (key_view == RUNTIME_CONFIG_ARG) [[likely]] {
      parse_runtime_config_arg(value_view);
    } else {
      kphp::log::warning("unexpected argument format: {}", key_view);
    }
  }
  runtime_config.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ini_opts.set_reference_counter_to(ExtraRefCnt::for_global_const);
}
