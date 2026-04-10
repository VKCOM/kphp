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
#include <sys/mman.h>
#include <sys/stat.h>
#include <utility>

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
  auto canonicalized_path{k2::canonicalize(value_view)};
  if (!canonicalized_path) [[unlikely]] {
    kphp::log::error("error canonicalizing runtime-config path: error_code -> {}, path -> {}", canonicalized_path.error(), value_view);
  }

  const auto [runtime_config_path, runtime_config_path_size]{*std::move(canonicalized_path)};
  auto runtime_config_file{kphp::fs::file::open({runtime_config_path.get(), runtime_config_path_size}, "r")};
  if (!runtime_config_file) [[unlikely]] {
    kphp::log::error("error opening runtime-config: error code -> {}, path -> {}", runtime_config_file.error(), value_view);
  }

  struct stat stat {};
  if (auto stat_result{k2::fstat(runtime_config_file->descriptor(), std::addressof(stat))}; !stat_result.has_value()) [[unlikely]] {
    kphp::log::error("error getting runtime-config stat: error code -> {}, path -> {}", stat_result.error(), value_view);
  }

  auto mmap{kphp::fs::mmap::create(stat.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, runtime_config_file->descriptor(), 0)};
  if (!mmap) [[unlikely]] {
    kphp::log::error("error mmaping runtime-config: error code -> {}, path -> {}", mmap.error(), value_view);
  }

  if (auto madvise_result{mmap->madvise(MADV_SEQUENTIAL)}; !madvise_result) [[unlikely]] {
    kphp::log::warning("error performing madvise on runtime-config's mmap: error code -> {}", madvise_result.error());
  }

  auto opt_config{json_decode({reinterpret_cast<const char*>(mmap->data().data()), mmap->data().size()})};
  if (!opt_config) [[unlikely]] {
    kphp::log::error("error decoding runtime-config: path -> {}", value_view);
  }
  runtime_config = *std::move(opt_config);
}

void ComponentState::parse_cluster_name(std::string_view value_view) noexcept {
  cluster_name = string{value_view.data(), static_cast<string::size_type>(value_view.size())};
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
    } else if (key_view == CLUSTER_NAME_ARG) [[likely]] {
      parse_cluster_name(value_view);
    } else {
      kphp::log::warning("unexpected argument format: {}", key_view);
    }
  }
}
