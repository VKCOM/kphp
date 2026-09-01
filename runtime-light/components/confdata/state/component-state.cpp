// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/component-state.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <system_error>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto ComponentState::parse_confdata_memory_limit_arg(std::string_view value_view) noexcept -> void {
  size_t parsed{};
  const auto [end, error]{std::from_chars(value_view.begin(), value_view.end(), parsed)};
  if (value_view.empty() || error != std::errc{} || end != value_view.end() || parsed == 0) [[unlikely]] {
    kphp::log::error("{} must be a positive integer, got '{}'", CONFDATA_MEMORY_LIMIT_ARG, value_view);
  }
  m_confdata_memory_limit = parsed;
}

auto ComponentState::parse_confdata_proxy_actor_name_arg(std::string_view value_view) noexcept -> void {
  m_confdata_proxy_actor_name = value_view;
}

auto ComponentState::parse_predefined_wildcards_arg(std::string_view value_view) noexcept -> void {
  m_predefined_wildcards.clear();
  m_predefined_wildcards_storage.assign(value_view);

  const std::string_view storage_view{m_predefined_wildcards_storage};
  size_t line_number{1};
  size_t line_begin{};
  while (line_begin < storage_view.size()) {
    const size_t line_end{storage_view.find('\n', line_begin)};
    const auto wildcard{storage_view.substr(line_begin, line_end - line_begin)};
    if (wildcard.empty()) [[unlikely]] {
      kphp::log::error("{} contains an empty line: line -> {}", PREDEFINED_WILDCARDS_ARG, line_number);
    }
    if (wildcard.contains('\r')) [[unlikely]] {
      kphp::log::error("{} contains a carriage return: line -> {}", PREDEFINED_WILDCARDS_ARG, line_number);
    }
    if (const auto validated{kphp::confdata::validate_predefined_wildcard(wildcard)}; !validated) [[unlikely]] {
      kphp::log::error("{} contains an invalid wildcard: line -> {}, error -> {}", PREDEFINED_WILDCARDS_ARG, line_number, validated.error());
    }
    m_predefined_wildcards.emplace_back(wildcard);

    if (line_end == std::string_view::npos) {
      break;
    }
    line_begin = line_end + 1;
    ++line_number;
  }
  if (!storage_view.empty() && storage_view.back() == '\n') [[unlikely]] {
    kphp::log::error("{} contains an empty trailing line; use the YAML '|-' block style", PREDEFINED_WILDCARDS_ARG);
  }

  std::ranges::sort(m_predefined_wildcards);
  m_predefined_wildcards.erase(std::ranges::unique(m_predefined_wildcards).begin(), m_predefined_wildcards.end());
}

auto ComponentState::parse_initial_instance_memory_size_arg(std::string_view value_view) noexcept -> void {
  size_t parsed{};
  const auto [end, error]{std::from_chars(value_view.begin(), value_view.end(), parsed)};
  if (value_view.empty() || error != std::errc{} || end != value_view.end() || parsed == 0) [[unlikely]] {
    kphp::log::error("{} must be a positive integer, got '{}'", INITIAL_INSTANCE_MEMORY_SIZE_ARG, value_view);
  }
  m_initial_instance_memory_size = parsed;
}

auto ComponentState::parse_min_instance_extra_memory_size_arg(std::string_view value_view) noexcept -> void {
  size_t parsed{};
  const auto [end, error]{std::from_chars(value_view.begin(), value_view.end(), parsed)};
  if (value_view.empty() || error != std::errc{} || end != value_view.end() || parsed == 0) [[unlikely]] {
    kphp::log::error("{} must be a positive integer, got '{}'", MIN_INSTANCE_EXTRA_MEMORY_SIZE_ARG, value_view);
  }
  m_min_instance_extra_memory_size = parsed;
}

auto ComponentState::parse_args() noexcept -> void {
  for (auto i{0}; i < m_argc; ++i) {
    const auto [arg_key, arg_value]{k2::arg_fetch(i)};
    const std::string_view key_view{arg_key.get(), std::strlen(arg_key.get())};
    const std::string_view value_view{arg_value.get(), std::strlen(arg_value.get())};

    if (key_view == CONFDATA_MEMORY_LIMIT_ARG) {
      parse_confdata_memory_limit_arg(value_view);
    } else if (key_view == CONFDATA_PROXY_ACTOR_NAME_ARG) {
      parse_confdata_proxy_actor_name_arg(value_view);
    } else if (key_view == PREDEFINED_WILDCARDS_ARG) {
      parse_predefined_wildcards_arg(value_view);
    } else if (key_view == INITIAL_INSTANCE_MEMORY_SIZE_ARG) {
      parse_initial_instance_memory_size_arg(value_view);
    } else if (key_view == MIN_INSTANCE_EXTRA_MEMORY_SIZE_ARG) {
      parse_min_instance_extra_memory_size_arg(value_view);
    } else {
      kphp::log::error("unexpected argument: {}", key_view);
    }
  }
}
