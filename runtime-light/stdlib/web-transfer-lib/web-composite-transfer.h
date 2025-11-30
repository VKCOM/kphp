// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <utility>

#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/details/web-error.h"
#include "runtime-light/stdlib/web-transfer-lib/web-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web {

inline auto composite_transfer_open(transfer_backend backend) noexcept -> kphp::coro::task<std::expected<composite_transfer, error>> {
  auto& web_state{WebInstanceState::get()};

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  tl::CompositeWebTransferOpen web_transfer_open{.web_backend = {static_cast<tl::CompositeWebTransferOpen::web_backend_type::underlying_type>(backend)}};
  tl::storer tls{web_transfer_open.footprint()};
  web_transfer_open.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  const auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request for Composite descriptor creation");
  }

  tl::Either<tl::CompositeWebTransferOpenResultOk, tl::WebError> transfer_open_resp{};
  tl::fetcher tlf{*resp};
  if (!transfer_open_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response for Composite descriptor creation");
  }

  const auto result{transfer_open_resp.value};
  if (std::holds_alternative<tl::WebError>(result)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(result))};
  }

  const auto descriptor{std::get<tl::CompositeWebTransferOpenResultOk>(result).descriptor.value};

  auto& composite2config{web_state.composite_transfer2config};
  kphp::log::assertion(composite2config.contains(descriptor) == false); // NOLINT
  composite2config.emplace(descriptor, composite_transfer_config{});

  auto& composite2simple_transfers{web_state.composite_transfer2simple_transfers};
  kphp::log::assertion(composite2simple_transfers.contains(descriptor) == false); // NOLINT
  composite2simple_transfers.emplace(descriptor, simple_transfers{});

  co_return std::expected<composite_transfer, error>{descriptor};
}

inline auto composite_transfer_add(composite_transfer ct, simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto& web_state{WebInstanceState::get()};

  auto& composite2config{web_state.composite_transfer2config};
  if (!composite2config.contains(ct.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Composite transfer"}}};
  }

  auto& simple2config{web_state.simple_transfer2config};
  if (!simple2config.contains(st.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("failed to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  // Ensure that simple transfer hasn't been added in some composite yet
  auto& simple_transfers{web_state.composite_transfer2simple_transfers[ct.descriptor]};
  if (simple_transfers.contains(st.descriptor)) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"the Composite transfer already includes this Simple transfer"}}};
  }
  simple_transfers.emplace(st.descriptor);

  // Set a holder for simple transfer
  auto& composite_holder{web_state.simple_transfer2holder[st.descriptor]};
  if (composite_holder.has_value()) {
    co_return std::unexpected{
        error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"this Simple transfer is already part of another Composite transfer"}}};
  }
  composite_holder.emplace(ct.descriptor);

  // Prepare simple config
  kphp::stl::vector<tl::webProperty, kphp::memory::script_allocator> tl_simple_props{};
  auto& props{simple2config[st.descriptor].properties};
  for (const auto& [id, val] : props) {
    tl::webProperty p{.id = tl::u64{id}, .value = val.serialize()};
    tl_simple_props.emplace_back(std::move(p));
  }
  tl::simpleWebTransferConfig tl_simple_config{tl::vector<tl::webProperty>{std::move(tl_simple_props)}};
  // Prepare `CompositeWebTransferAdd` method
  tl::CompositeWebTransferAdd tl_add{
      .composite_descriptor = tl::u64{ct.descriptor}, .simple_descriptor = tl::u64{st.descriptor}, .simple_config = std::move(tl_simple_config)};
  tl::storer tls{tl_add.footprint()};
  tl_add.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of adding Simple into Composite transfer");
  }

  tl::Either<tl::CompositeWebTransferAddResultOk, tl::WebError> composite_add_resp{};
  tl::fetcher tlf{*resp};
  if (!composite_add_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of adding Simple into Composite transfer");
  }

  if (auto r{composite_add_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(r))};
  }

  co_return std::expected<void, error>{};
}

inline auto composite_transfer_remove(composite_transfer ct, simple_transfer st) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto& web_state{WebInstanceState::get()};

  auto& composite2config{web_state.composite_transfer2config};
  if (!composite2config.contains(ct.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Composite transfer"}}};
  }

  auto& simple2config{web_state.simple_transfer2config};
  if (!simple2config.contains(st.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Simple transfer"}}};
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("fastruct composite_transfer_config;iled to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  // Ensure that simple transfer alreadt has been added in some composite
  auto& simple_transfers{web_state.composite_transfer2simple_transfers[ct.descriptor]};
  if (!simple_transfers.contains(st.descriptor)) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"this Composite transfer does not include this Simple transfer"}}};
  }
  simple_transfers.erase(st.descriptor);

  // Unset a holder for simple transfer
  auto& composite_holder{web_state.simple_transfer2holder[st.descriptor]};
  if (!composite_holder.has_value()) {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"this Simple transfer is not yet part of any Composite transfer"}}};
  }
  composite_holder.reset();

  // Prepare `CompositeWebTransferRemove` method
  tl::CompositeWebTransferRemove tl_remove{.composite_descriptor = tl::u64{ct.descriptor}, .simple_descriptor = tl::u64{st.descriptor}};
  tl::storer tls{tl_remove.footprint()};
  tl_remove.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of removing Simple into Composite transfer");
  }

  tl::Either<tl::CompositeWebTransferRemoveResultOk, tl::WebError> composite_remove_resp{};
  tl::fetcher tlf{*resp};
  if (!composite_remove_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of removing Simple into Composite transfer");
  }

  if (auto r{composite_remove_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(r))};
  }

  co_return std::expected<void, error>{};
}

inline auto composite_transfer_close(composite_transfer ct) noexcept -> kphp::coro::task<std::expected<void, error>> {
  auto& web_state{WebInstanceState::get()};

  auto& composite2config{web_state.composite_transfer2config};
  if (!composite2config.contains(ct.descriptor)) [[unlikely]] {
    co_return std::unexpected{error{.code = WEB_INTERNAL_ERROR_CODE, .description = string{"unknown Composite transfer"}}};
  }

  auto session{web_state.session_get_or_init()};
  if (!session.has_value()) [[unlikely]] {
    kphp::log::error("fastruct composite_transfer_config;iled to start or get session with Web component");
  }

  if ((*session).get() == nullptr) [[unlikely]] {
    kphp::log::error("session with Web components has been closed");
  }

  // Enumerate over all included simple transfers and close them
  auto& simple_transfers{web_state.composite_transfer2simple_transfers[ct.descriptor]};
  for (const auto st : simple_transfers) {
    if (auto remove_res{co_await kphp::web::composite_transfer_remove(ct, kphp::web::simple_transfer{st})}; !remove_res.has_value()) {
      co_return std::move(remove_res);
    };
  }
  simple_transfers.clear();

  tl::CompositeWebTransferClose tl_close{tl::u64{ct.descriptor}};
  tl::storer tls{tl_close.footprint()};
  tl_close.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{[&resp_buf](size_t size) noexcept -> std::span<std::byte> {
    resp_buf.resize(size);
    return {resp_buf.data(), size};
  }};

  auto resp{co_await (*session).get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) [[unlikely]] {
    kphp::log::error("failed to send request of closing Composite transfer");
  }

  tl::Either<tl::CompositeWebTransferCloseResultOk, tl::WebError> composite_close_resp{};
  tl::fetcher tlf{*resp};
  if (!composite_close_resp.fetch(tlf)) [[unlikely]] {
    kphp::log::error("failed to parse response of closing Composite transfer");
  }

  if (auto r{composite_close_resp.value}; std::holds_alternative<tl::WebError>(r)) {
    co_return std::unexpected{details::process_error(std::get<tl::WebError>(r))};
  }

  co_return std::expected<void, error>{};
}

} // namespace kphp::web
