// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/component/inter-component-session/client.h"
#include "runtime-light/stdlib/web/defs.h"

inline constexpr std::string_view WEB_COMPONENT_NAME{"web"};

struct WebInstanceState final : private vk::not_copyable {

  struct WebComponentSession : public refcountable_php_classes<WebComponentSession> {
    kphp::component::inter_component_session::client client;
  };
  using shared_session_type = class_instance<WebComponentSession>;

  std::optional<shared_session_type> session{};
  bool session_is_finished{false};
  kphp::stl::unordered_map<kphp::web::SimpleTransfer, kphp::web::SimpleTransferConfig, kphp::memory::script_allocator> simple_transfer2config{};

  inline auto session_get_or_init() noexcept -> std::expected<shared_session_type, int32_t>;

  WebInstanceState() noexcept = default;

  static WebInstanceState& get() noexcept;
};

inline auto WebInstanceState::session_get_or_init() noexcept -> std::expected<shared_session_type, int32_t> {
  if (session_is_finished) {
    return std::unexpected{k2::errno_eshutdown};
  }
  if (session.has_value()) {
    return std::expected<shared_session_type, int32_t>{session.value()};
  }
  auto expected{kphp::component::inter_component_session::client::create(WEB_COMPONENT_NAME)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  session.emplace(make_instance<WebComponentSession>(WebComponentSession{.client = std::move(expected.value())}));
  return std::expected<shared_session_type, int32_t>{session.value()};
}
