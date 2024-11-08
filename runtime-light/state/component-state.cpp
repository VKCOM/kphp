// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/component-state.h"

#include <cstring>
#include <iterator>
#include <string_view>

#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/json-functions.h"

void ComponentState::parse_ini_arg(std::string_view key_view, std::string_view value_view) noexcept {
  if (key_view.size() <= INI_ARG_PREFIX.size()) [[unlikely]] {
    php_warning("wrong ini argument format %s", key_view.data());
    return;
  }
  const auto *key_ptr{key_view.data()};
  std::advance(key_ptr, INI_ARG_PREFIX.size());
  string key_str{key_ptr, static_cast<string::size_type>(key_view.size() - INI_ARG_PREFIX.size())};
  key_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

  string value_str{value_view.data(), static_cast<string::size_type>(value_view.size())};
  value_str.set_reference_counter_to(ExtraRefCnt::for_global_const);

  ini_opts.set_value(key_str, value_str);
}

void ComponentState::parse_runtime_config_arg(std::string_view value_view) noexcept {
  // FIXME: actually no need to allocate string here
  auto [config, ok]{json_decode(string{value_view.data(), static_cast<string::size_type>(value_view.size())})};
  if (ok) [[likely]] {
    runtime_config = std::move(config);
  } else {
    php_warning("runtime config is not a JSON");
  }
}

void ComponentState::parse_args() noexcept {
  for (auto i = 0; i < argc; ++i) {
    const auto [arg_key, arg_value]{k2::arg_fetch(i)};
    const std::string_view key_view{arg_key.get(), std::strlen(arg_key.get())};
    const std::string_view value_view{arg_value.get(), std::strlen(arg_value.get())};

    if (key_view.starts_with(INI_ARG_PREFIX)) {
      parse_ini_arg(key_view, value_view);
    } else if (key_view == RUNTIME_CONFIG_ARG) {
      parse_runtime_config_arg(value_view);
    } else {
      php_warning("unknown argument: %s", key_view.data());
    }
  }
  runtime_config.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ini_opts.set_reference_counter_to(ExtraRefCnt::for_global_const);
}
