// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

// === ComponentQuery =============================================================================

struct C$ComponentQuery final : public refcountable_php_classes<C$ComponentQuery> {
  uint64_t stream_d{k2::INVALID_PLATFORM_DESCRIPTOR};

  explicit constexpr C$ComponentQuery(uint64_t stream_d_) noexcept
    : stream_d(stream_d_) {}
  constexpr C$ComponentQuery(C$ComponentQuery &&other) noexcept
    : stream_d(std::exchange(other.stream_d, k2::INVALID_PLATFORM_DESCRIPTOR)) {};

  C$ComponentQuery(const C$ComponentQuery &) = delete;
  C$ComponentQuery &operator=(const C$ComponentQuery &) = delete;
  C$ComponentQuery &operator=(C$ComponentQuery &&other) = delete;

  constexpr const char *get_class() const noexcept {
    return "ComponentQuery";
  }

  constexpr int32_t get_hash() const noexcept {
    return static_cast<int32_t>(std::hash<std::string_view>{}(get_class()));
  }

  ~C$ComponentQuery() {
    auto &instance_st{InstanceState::get()};
    if (instance_st.opened_streams().contains(stream_d)) {
      instance_st.release_stream(stream_d);
    }
  }
};

// === component query client interface ===========================================================

task_t<class_instance<C$ComponentQuery>> f$component_client_send_request(string name, string message) noexcept;

task_t<string> f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept;

// === component query server interface ===========================================================

task_t<class_instance<C$ComponentQuery>> f$component_server_accept_query() noexcept;

task_t<string> f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept;

task_t<void> f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept;
