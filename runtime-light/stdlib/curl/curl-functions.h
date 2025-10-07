// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/component/inter-component-session/client.h"
#include "runtime-light/stdlib/web/web.h"

using curl_easy = kphp::web::SimpleTransfer;

inline constexpr std::string_view COMPONENT_NAME{"web"};

inline Optional<array<int64_t>> f$curl_multi_info_read(int64_t /*unused*/,
                                                       int64_t& /*unused*/ = CurlInstanceState::get().curl_multi_info_read_msgs_in_queue_stub) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}

inline bool f$curl_setopt_array(int64_t /*unused*/, const array<mixed>& /*unused*/) noexcept {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}

struct SharedClient: public refcountable_php_classes<SharedClient> {
  kphp::component::inter_component_session::Client c;
};

inline auto f$curl_init() noexcept -> kphp::coro::task<curl_easy> {
  auto res {co_await kphp::web::simple_transfer_open(kphp::web::TransferBackend::CURL)};
  if (!res.has_value()) {
    php_warning("Could not set url to a new curl easy handle %s", res.error().description->c_str());
    co_return 0;
  }
  co_return res.value();
}

inline auto f$curl_setopt(curl_easy easy_id, int64_t option, const mixed& value) noexcept -> bool {
  if (auto expected {kphp::web::set_transfer_prop(easy_id, option, value)}; !expected) {
    php_warning("Some error during setopt");
    return false;
  }
  return true;
}


inline auto f$curl_exec(int64_t easy_id) noexcept -> kphp::coro::task<mixed> {

//  auto shared_client{make_instance<SharedClient>(SharedClient{.c = std::move(kphp::component::inter_component_session::Client::create(COMPONENT_NAME).value())})};
//
//  for (size_t i = 0; i < 5; ++i) {
//    kphp::coro::io_scheduler::get().start([](class_instance<SharedClient> sc) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
//      kphp::stl::vector<std::byte, kphp::memory::script_allocator> data{static_cast<std::byte>(1), static_cast<std::byte>(2), static_cast<std::byte>(3)};
//      std::span<const std::byte> req = data;
//
//      string headers{};
//      string body{};
//
//      auto t{0};
//      kphp::component::inter_component_session::BufferProvider response_buffer_provider{[&t, &headers, &body]() noexcept {
//        if (t == 0) {
//          t += 1;
//          return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&headers](size_t size) -> std::span<std::byte> {
//            headers.reserve_at_least(size);
//            return {reinterpret_cast<std::byte*>(headers.buffer()), size};
//          }};
//        } else {
//          return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&body](size_t size) -> std::span<std::byte> {
//            body.reserve_at_least(size);
//            return {reinterpret_cast<std::byte*>(body.buffer()), size};
//          }};
//        }
//      }};
//
//      auto res{co_await std::move(
//          sc.get()->c.query(req, std::move(response_buffer_provider), [&t](std::span<std::byte> resp) -> kphp::coro::task<bool> { co_return (++t != 2); }))};
//      kphp::log::info("--> {} <--", headers.c_str());
//      kphp::log::info("--> {} <--", body.c_str());
//      co_return res;
//
//    }(shared_client));
//  }

  auto res{co_await kphp::web::simple_transfer_perform(easy_id)};
  if (!res.has_value()) {
    php_warning("Perform error: %s", res.error().description.value().c_str());
    co_return false;
  }
  co_return res.value().body;
}
