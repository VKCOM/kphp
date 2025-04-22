// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/component-state.h"

#include <cctype>
#include <cstring>
#include <iterator>
#include <string_view>
#include <utility>

#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-light/core/compiler-info/compiler_interface.h"
#include "runtime-light/k2-platform/k2-api.h"

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
    php_warning("wrong ini argument format %s", key_view.data());
    return;
  }
  string key_str{std::next(key_view.data(), INI_ARG_PREFIX.size()), static_cast<string::size_type>(key_view.size() - INI_ARG_PREFIX.size())};
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

void ComponentState::parse_command_line_arg(std::string_view value_view) noexcept {
  if (value_view.empty()) [[unlikely]] {
    php_warning("command line argument is empty");
    return;
  }

  if (!command_line_argv.empty()) [[unlikely]] {
    php_warning("command line argument support no more one usage");
    return;
  }

  const auto & main_file_view{kphp::compiler_interface::get_main_file_name()};
  command_line_argv.push_back(string(main_file_view.data()));

  bool in_quote{};
  string current_arg{};
  for (char letter : value_view) {
    if (std::isspace(letter) && !in_quote && !current_arg.empty()) {
      command_line_argv.push_back(std::move(current_arg));
      current_arg = string();
    } else if (letter == '"') {
      in_quote = !in_quote;
    } else if (letter == '\'') {
      php_warning("in command line arg supported only \" quote");
    } else {
      current_arg.push_back(letter);
    }
  }

  if (!current_arg.empty()) {
    command_line_argv.push_back(std::move(current_arg));
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
    } else if (key_view == COMMAND_LINE_ARG) {
      parse_command_line_arg(value_view);
    } else {
      php_warning("unknown argument: %s", key_view.data());
    }
  }
  command_line_argv.set_reference_counter_to(ExtraRefCnt::for_global_const);
  runtime_config.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ini_opts.set_reference_counter_to(ExtraRefCnt::for_global_const);
}
