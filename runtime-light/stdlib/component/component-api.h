// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/stream.h"

// === ComponentQuery =============================================================================

class C$ComponentQuery final : public refcountable_php_classes<C$ComponentQuery> {
  kphp::component::stream m_stream;

public:
  explicit C$ComponentQuery(kphp::component::stream stream) noexcept
      : m_stream(std::move(stream)) {}
  C$ComponentQuery(C$ComponentQuery&& other) noexcept
      : m_stream(std::move(other.m_stream)) {}
  ~C$ComponentQuery() = default;

  C$ComponentQuery(const C$ComponentQuery&) = delete;
  C$ComponentQuery& operator=(const C$ComponentQuery&) = delete;
  C$ComponentQuery& operator=(C$ComponentQuery&& other) = delete;

  constexpr const char* get_class() const noexcept {
    return "ComponentQuery";
  }

  constexpr int32_t get_hash() const noexcept {
    std::string_view name_view{C$ComponentQuery::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  auto stream() noexcept -> kphp::component::stream& {
    return m_stream;
  }
};

// === component query client interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_client_send_request(string name, string message) noexcept;

kphp::coro::task<string> f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept;

// === component query server interface ===========================================================

kphp::coro::task<class_instance<C$ComponentQuery>> f$component_server_accept_query() noexcept;

kphp::coro::task<string> f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept;

kphp::coro::task<> f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept;
