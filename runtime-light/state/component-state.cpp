// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/component-state.h"

#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <sys/stat.h>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/resource.h"

void ComponentState::parse_env() noexcept {
  for (auto i = 0; i < envc; ++i) {
    const auto [env_key, env_value]{k2::env_fetch(i)};
    env.set_value(string{env_key.get()}, string{env_value.get()});
  }
}

void ComponentState::parse_ini_arg(std::string_view key_view, std::string_view value_view) noexcept {
  if (key_view.size() <= INI_ARG_PREFIX.size()) [[unlikely]] {
    kphp::log::warning("bad ini argument: {}", key_view);
    return;
  }
  string key_str{std::next(key_view.data(), INI_ARG_PREFIX.size()), static_cast<string::size_type>(key_view.size() - INI_ARG_PREFIX.size())};
  string value_str{value_view.data(), static_cast<string::size_type>(value_view.size())};
  ini_opts.set_value(key_str, value_str);
}

void ComponentState::parse_kml_arg(std::string_view kml_dir) noexcept {
  kml_component_state.init(kml_dir);
}

void ComponentState::parse_runtime_config_arg(std::string_view value_view) noexcept {
  auto expected_canonicalized_path{k2::canonicalize(value_view)};
  if (!expected_canonicalized_path) [[unlikely]] {
    return kphp::log::warning("error canonicalizing runtime-config path: error_code -> {}, path -> '{}'", expected_canonicalized_path.error(), value_view);
  }

  const auto [runtime_config_path, runtime_config_path_size]{*std::move(expected_canonicalized_path)};
  auto expected_runtime_config_file{kphp::fs::file::open({runtime_config_path.get(), runtime_config_path_size}, "r")};
  if (!expected_runtime_config_file) [[unlikely]] {
    return kphp::log::warning("error opening runtime-config: error code -> {}", expected_runtime_config_file.error());
  }

  struct stat stat {};
  if (auto error_code{k2::stat({runtime_config_path.get(), runtime_config_path_size}, std::addressof(stat))}; error_code != k2::errno_ok) [[unlikely]] {
    return kphp::log::warning("error getting runtime-config stat: error code -> {}", error_code);
  }

  const auto runtime_config_mem{std::unique_ptr<char, decltype(std::addressof(kphp::memory::script::free))>{
      reinterpret_cast<char*>(kphp::memory::script::alloc(static_cast<size_t>(stat.st_size))), kphp::memory::script::free}};
  kphp::log::assertion(runtime_config_mem != nullptr);
  auto runtime_config_buf{std::span<char>{runtime_config_mem.get(), static_cast<size_t>(stat.st_size)}};
  if (auto expected_read{(*expected_runtime_config_file).read(std::as_writable_bytes(runtime_config_buf))}; !expected_read || *expected_read != stat.st_size)
      [[unlikely]] {
    return kphp::log::warning("error reading runtime-config: error code -> {}, read bytes -> {}", expected_read.error_or(k2::errno_ok),
                              expected_read.value_or(0));
  }

  auto opt_config{json_decode({runtime_config_buf.data(), runtime_config_buf.size()})};
  if (!opt_config) [[unlikely]] {
    return kphp::log::warning("error decoding runtime-config");
  }
  runtime_config = *std::move(opt_config);
}

void ComponentState::parse_args() noexcept {
  for (auto i = 0; i < argc; ++i) {
    const auto [arg_key, arg_value]{k2::arg_fetch(i)};
    const std::string_view key_view{arg_key.get(), std::strlen(arg_key.get())};
    const std::string_view value_view{arg_value.get(), std::strlen(arg_value.get())};

    if (key_view.starts_with(INI_ARG_PREFIX)) {
      parse_ini_arg(key_view, value_view);
    } else if (key_view == KML_DIR_ARG) {
      parse_kml_arg(value_view);
    } else if (key_view == RUNTIME_CONFIG_ARG) [[likely]] {
      parse_runtime_config_arg(value_view);
    } else {
      kphp::log::warning("unexpected argument format: {}", key_view);
    }
  }
}
