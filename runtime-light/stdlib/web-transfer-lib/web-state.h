// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <expected>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/component/inter-component-session/client.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"

inline constexpr std::string_view WEB_COMPONENT_NAME{"web"};

struct WebInstanceState final : private vk::not_copyable {

  struct WebComponentSession : public refcountable_php_classes<WebComponentSession> {
    kphp::component::inter_component_session::client client;
  };
  using shared_session_type = class_instance<WebComponentSession>;

  std::optional<shared_session_type> session{};
  bool session_is_finished{false};
  kphp::stl::unordered_map<kphp::web::simple_transfer::descriptor_type, kphp::web::simple_transfer_config, kphp::memory::script_allocator>
      simple_transfer2config{};
  kphp::stl::map<kphp::web::simple_transfer::descriptor_type, std::optional<kphp::web::composite_transfer::descriptor_type>, kphp::memory::script_allocator>
      simple_transfer2holder{};

  kphp::stl::unordered_map<kphp::web::composite_transfer::descriptor_type, kphp::web::composite_transfer_config, kphp::memory::script_allocator>
      composite_transfer2config{};
  kphp::stl::map<kphp::web::simple_transfer::descriptor_type, kphp::web::simple_transfers, kphp::memory::script_allocator> composite_transfer2simple_transfers{};

  inline auto session_get_or_init() noexcept -> std::expected<shared_session_type, int32_t>;

  WebInstanceState() noexcept = default;

  static WebInstanceState& get() noexcept;
};

inline auto WebInstanceState::session_get_or_init() noexcept -> std::expected<shared_session_type, int32_t> {
  if (session_is_finished) {
    return std::unexpected{k2::errno_eshutdown};
  }
  if (session.has_value()) {
    return std::expected<shared_session_type, int32_t>{*session};
  }
  auto expected{kphp::component::inter_component_session::client::create(WEB_COMPONENT_NAME)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  session.emplace(make_instance<WebComponentSession>(WebComponentSession{.client = std::move(*expected)}));
  return std::expected<shared_session_type, int32_t>{*session};
}
