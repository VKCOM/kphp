// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/web/defs.h"

namespace kphp::web {

inline auto simple_transfer_open(TransferBackend backend) noexcept -> kphp::coro::task<std::expected<SimpleTransfer, Error>> {
  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) {
    co_return std::unexpected{Error{.code = session.error(), .description = string("Failed to start or get session with Web component")}};
  }

  tl::SimpleWebTransferOpen web_transfer_open{.web_backend = {static_cast<tl::SimpleWebTransferOpen::web_backend_type::underlying_type>(backend)}};
  tl::storer tls{web_transfer_open.footprint()};
  web_transfer_open.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> resp_buf{};
  auto response_buffer_provider{kphp::component::inter_component_session::BufferProvider([&resp_buf]() noexcept {
    return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&resp_buf](size_t size) -> std::span<std::byte> {
      resp_buf.resize(size);
      return {resp_buf.data(), size};
    }};
  })};
  if (session.value().get() == nullptr) {
    co_return std::unexpected{Error{.code = k2::errno_eshutdown, .description = string("Session with Web components has been closed")}};
  }
  auto resp{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider))};
  if (!resp.has_value()) {
    co_return std::unexpected{Error{.code = resp.error(), .description = string("Failed to send request of Simple descriptor creation")}};
  }

  tl::SimpleWebTransferOpenResponse simple_web_transfer_resp{};
  tl::fetcher tlf{resp.value()};
  if (!simple_web_transfer_resp.fetch(tlf)) [[unlikely]] {
    co_return std::unexpected{Error{.code = k2::errno_einval, .description = string("Failed to parse response of Simple descriptor creation")}};
  }

  std::expected<SimpleTransfer, Error> result;
  std::visit(
      [&result](const auto& v) noexcept {
        using value_t = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::same_as<value_t, tl::SimpleWebTransferOpenResultOk>) {
          result = std::expected<SimpleTransfer, Error>{static_cast<tl::SimpleWebTransferOpenResultOk>(v).desc.value};
        } else {
          auto error{static_cast<tl::WebError>(v)};
          result = std::unexpected{Error{.code = error.code.value, .description = string(error.description.value.data(), error.description.value.size())}};
        }
      },
      simple_web_transfer_resp.value);
  co_return result;
}

inline auto set_transfer_prop(SimpleTransfer st, PropertyId prop_id, PropertyValue prop_value) -> std::expected<Ok, Error> {
  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  if (!simple2config.contains(st)) {
    simple2config.emplace(st, SimpleTransferConfig{});
  }
  simple2config[st].properties.insert({prop_id, prop_value});
  return std::expected<Ok, Error>{Ok{}};
}

inline auto property_value_serialize(PropertyValue value) noexcept -> tl::WebPropertyValue {
  switch (value.get_type()) {
  case mixed::type::BOOLEAN: {
    return tl::WebPropertyValue{.value = tl::Bool{value.as_bool()}};
  }
  case mixed::type::INTEGER: {
    return tl::WebPropertyValue{.value = tl::I64{tl::i64{value.as_int()}}};
  }
  case mixed::type::STRING: {
    auto& s{value.as_string()};
    return tl::WebPropertyValue{.value = tl::String{tl::string{{s.c_str(), s.size()}}}};
  }
  case mixed::type::ARRAY: {
    const auto& a{value.as_array()};
    if (a.is_vector()) {
      tl::Vector<tl::WebPropertyValue> res{tl::vector<tl::WebPropertyValue>{.value = {}}};
      for (const auto& i : a) {
        res.inner.value.emplace_back(property_value_serialize(i.get_value()));
      }
      return tl::WebPropertyValue{.value = std::move(res)};
    } else {
      tl::Dictionary<tl::WebPropertyValue> res{tl::Dictionary<tl::WebPropertyValue>{tl::vector<tl::dictionaryField<tl::WebPropertyValue>>{.value = {}}}};
      for (const auto& i : a) {
        kphp::log::assertion(i.get_key().is_string());
        const auto& key{i.get_key().as_string()};
        const auto& val{i.get_value()};
        res.inner.value.value.emplace_back(tl::dictionaryField{.key = tl::string{{key.c_str(), key.size()}}, .value = property_value_serialize(val)});
      }
      return tl::WebPropertyValue{.value = std::move(res)};
    }
  }
  default:
    // Unsupported kind
    kphp::log::assertion(false);
    return tl::WebPropertyValue{};
  }
}

inline auto simple_transfer_perform(SimpleTransfer st) noexcept -> kphp::coro::task<std::expected<Response, Error>> {
  auto& simple2config{WebInstanceState::get().simple_transfer2config};
  if (!simple2config.contains(st)) {
    co_return std::unexpected{Error{.code = k2::errno_enodata, .description = string("Unknown Simple descriptor")}};
  }

  auto session{WebInstanceState::get().session_get_or_init()};
  if (!session.has_value()) {
    co_return std::unexpected{Error{.code = session.error(), .description = string("Failed to start or get session with Web component")}};
  }

  if (session.value().get() == nullptr) {
    co_return std::unexpected{Error{.code = k2::errno_eshutdown, .description = string("Session with Web components has been closed")}};
  }

  kphp::stl::vector<tl::WebProperty, kphp::memory::script_allocator> tl_props{};
  auto& props{simple2config[st].properties};
  for (const auto& [id, val] : props) {
    tl::WebProperty p{.id = tl::u64{id}, .value = property_value_serialize(val)};
    kphp::log::debug("id {} ", id);
    tl_props.emplace_back(std::move(p));
  }
  tl::SimpleWebTransferConfig tl_config{tl::vector<tl::WebProperty>{std::move(tl_props)}};
  tl::SimpleWebTransferPerform tl_perform{.desc = tl::u64{st}, .config = std::move(tl_config)};
  tl::storer tls{tl_perform.footprint()};
  tl_perform.store(tls);

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> ok_or_error_buffer{};
  Response response{};
  std::optional<Error> error{};
  auto frame_num{0};

  kphp::component::inter_component_session::BufferProvider response_buffer_provider{[&frame_num, &ok_or_error_buffer, &response]() noexcept {
    if (frame_num == 0) {
      return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&ok_or_error_buffer](size_t size) -> std::span<std::byte> {
        ok_or_error_buffer.resize(size);
        return {ok_or_error_buffer.data(), size};
      }};
    } else if (frame_num == 1) {
      return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&response](size_t size) -> std::span<std::byte> {
        response.headers = string(size, false);
        return {reinterpret_cast<std::byte*>(response.headers.buffer()), size};
      }};
    } else {
      return kphp::component::inter_component_session::BufferProvider::SliceMaker{[&response](size_t size) -> std::span<std::byte> {
        response.body = string(size, false);
        return {reinterpret_cast<std::byte*>(response.body.buffer()), size};
      }};
    }
  }};

  const auto& response_handler{[&frame_num, &error, &ok_or_error_buffer](std::span<std::byte> _) -> kphp::coro::task<bool> {
    if (frame_num == 0) {
      frame_num++;
      tl::SimpleWebTransferPerformResponse simple_web_transfer_perform_resp{};
      tl::fetcher tlf{ok_or_error_buffer};
      if (!simple_web_transfer_perform_resp.fetch(tlf)) [[unlikely]] {
        error.emplace(Error{.code = k2::errno_einval, .description = string("Failed to parse response of Simple descriptor performing")});
        co_return false;
      }

      std::visit(
          [&error](const auto& v) noexcept {
            using value_t = std::remove_cvref_t<decltype(v)>;
            if constexpr (std::same_as<value_t, tl::WebError>) {
              auto tl_web_error{static_cast<tl::WebError>(v)};
              error.emplace(Error{.code = tl_web_error.code.value, .description = string(tl_web_error.description.value.data(), tl_web_error.description.value.size())});
            }
          },
          simple_web_transfer_perform_resp.value);

      if (error.has_value()) {
        co_return false;
      }
      co_return true;
    } else if (frame_num == 1) {
      frame_num++;
      co_return true;
    }
    co_return false;
  }};

  if (auto res{co_await session.value().get()->client.query(tls.view(), std::move(response_buffer_provider), response_handler)}; !res) {
    co_return std::unexpected{Error{.code = res.error(), .description = string("Failed to send request of Simple descriptor performing")}};
  }
  if (error.has_value()) {
    co_return std::unexpected{std::move(error.value())};
  }
  co_return std::expected<Response, Error>{std::move(response)};
}

inline auto simple_transfer_reset(SimpleTransfer st) noexcept -> kphp::coro::task<std::expected<Ok, Error>> {
  co_return std::unexpected{Error{.code = -1, .description = std::nullopt}};
}

inline auto simple_transfer_close(SimpleTransfer st) noexcept -> kphp::coro::task<std::expected<Ok, Error>> {
  co_return std::unexpected{Error{.code = -1, .description = std::nullopt}};
}

} // namespace kphp::web
