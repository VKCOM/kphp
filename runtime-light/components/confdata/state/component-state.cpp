// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/component-state.h"

#include <string_view>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto ComponentState::parse_confdata_proxy_actor_name_arg(std::string_view value_view) noexcept -> void {
  m_confdata_proxy_actor_name = value_view;
}

auto ComponentState::parse_args() noexcept -> void {
  for (auto i = 0; i < m_argc; ++i) {
    const auto [arg_key, arg_value]{k2::arg_fetch(i)};
    const std::string_view key_view{arg_key.get(), std::strlen(arg_key.get())};
    const std::string_view value_view{arg_value.get(), std::strlen(arg_value.get())};

    if (key_view == CONFDATA_PROXY_ACTOR_NAME_ARG) {
      parse_confdata_proxy_actor_name_arg(value_view);
    } else {
      kphp::log::error("unexpected argument: {}", key_view);
    }
  }
}
