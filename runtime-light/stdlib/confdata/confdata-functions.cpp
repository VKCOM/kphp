// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-functions.h"

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-common/stdlib/serialization/serialize-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/confdata/confdata-constants.h"
#include "runtime-light/stdlib/confdata/confdata-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

mixed extract_confdata_value(const tl::confdataValue& confdata_value) noexcept {
  if (confdata_value.is_php_serialized.value && confdata_value.is_json_serialized.value) [[unlikely]] { // check that we don't have both flags set
    kphp::log::warning("confdata value has both php_serialized and json_serialized flags set");
    return {};
  }
  if (confdata_value.is_php_serialized.value) {
    return unserialize_raw(confdata_value.value.value.data(), static_cast<string::size_type>(confdata_value.value.value.size()));
  } else if (confdata_value.is_json_serialized.value) {
    return json_decode(confdata_value.value.value).value_or(mixed{});
  } else {
    return string{confdata_value.value.value.data(), static_cast<string::size_type>(confdata_value.value.value.size())};
  }
}

} // namespace

kphp::coro::task<mixed> f$confdata_get_value(const string& key) noexcept {
  auto& confdata_key_cache{ConfdataInstanceState::get().key_cache()};
  if (auto it{confdata_key_cache.find(key)}; it != confdata_key_cache.end()) {
    co_return it->second;
  }

  tl::ConfdataGet confdata_get{.key = {.value = {key.c_str(), key.size()}}};
  tl::storer tls{confdata_get.footprint()};
  confdata_get.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::confdata::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return mixed{};
  }

  auto stream{*std::move(expected_stream)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), kphp::component::read_ext::append(response)))) [[unlikely]] {
    co_return mixed{};
  }

  tl::fetcher tlf{response};
  tl::Maybe<tl::confdataValue> maybe_confdata_value{};
  kphp::log::assertion(maybe_confdata_value.fetch(tlf));

  if (!maybe_confdata_value.opt_value) { // no such key
    co_return mixed{};
  }

  auto value{extract_confdata_value(*maybe_confdata_value.opt_value)}; // the key exists
  confdata_key_cache.emplace(key, value);
  co_return std::move(value);
}

kphp::coro::task<array<mixed>> f$confdata_get_values_by_any_wildcard(const string& wildcard) noexcept {
  static constexpr size_t CONFDATA_GET_WILDCARD_STREAM_CAPACITY = 1 << 20;

  auto& confdata_wildcard_cache{ConfdataInstanceState::get().wildcard_cache()};
  if (auto it{confdata_wildcard_cache.find(wildcard)}; it != confdata_wildcard_cache.end()) {
    co_return it->second;
  }

  tl::ConfdataGetWildcard confdata_get_wildcard{.wildcard = {.value = {wildcard.c_str(), wildcard.size()}}};
  tl::storer tls{confdata_get_wildcard.footprint()};
  confdata_get_wildcard.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::confdata::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return array<mixed>{};
  }

  auto stream{*std::move(expected_stream)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response{};
  response.reserve(CONFDATA_GET_WILDCARD_STREAM_CAPACITY);
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), kphp::component::read_ext::append(response)))) [[unlikely]] {
    co_return array<mixed>{};
  }

  tl::fetcher tlf{response};
  tl::Dictionary<tl::confdataValue> dict_confdata_value{};
  kphp::log::assertion(dict_confdata_value.fetch(tlf));

  array<mixed> result{array_size{static_cast<int64_t>(dict_confdata_value.size()), false}};
  std::for_each(dict_confdata_value.begin(), dict_confdata_value.end(), [&result](const auto& dict_field) noexcept {
    result.set_value(string{dict_field.key.value.data(), static_cast<string::size_type>(dict_field.key.value.size())},
                     extract_confdata_value(dict_field.value));
  });
  confdata_wildcard_cache.emplace(wildcard, result);
  co_return std::move(result);
}
