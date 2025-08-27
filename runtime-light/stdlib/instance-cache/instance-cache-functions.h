// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::instance_cache::details {

inline constexpr std::string_view COMPONENT_NAME{"instance_cache"};

} // namespace kphp::instance_cache::details

template<typename InstanceType>
kphp::coro::task<bool> f$instance_cache_store(string key, InstanceType instance, int64_t ttl = 0) noexcept {
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("ttl can't be negative: ttl -> {}, key -> {}", ttl, key.c_str());
    co_return false;
  }
  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    kphp::log::warning("ttl exceeds maximum allowed value, key will be stored forever: ttl -> {}, max -> {}, key -> {}", ttl,
                       std::numeric_limits<uint32_t>::max(), key.c_str());
    ttl = 0;
  }

  auto serialized_instance{f$instance_serialize(instance)};
  if (!serialized_instance.has_value()) [[unlikely]] {
    kphp::log::warning("can't serialize instance: key -> {}", key.c_str());
    co_return false;
  }

  tl::CacheStore cache_store{.key = tl::string{.value = {key.c_str(), key.size()}},
                             .value = tl::string{.value = {serialized_instance.val().c_str(), serialized_instance.val().size()}},
                             .ttl = tl::u32{.value = static_cast<uint32_t>(ttl)}};
  tl::storer tls{cache_store.footprint()};
  cache_store.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::instance_cache::details::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return false;
  }

  auto stream{*std::move(expected_stream)};
  std::array<std::byte, tl::Bool{}.footprint()> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), response))) [[unlikely]] {
    co_return false;
  }

  tl::Bool tl_bool{};
  tl::fetcher tlf{response};
  kphp::log::assertion(tl_bool.fetch(tlf));
  InstanceCacheInstanceState::get().request_cache.emplace(std::move(key), std::move(instance));
  co_return tl_bool.value;
}

template<typename InstanceType>
kphp::coro::task<InstanceType> f$instance_cache_fetch(string /*class_name*/, string key, bool /*even_if_expired*/ = false) noexcept {
  auto& request_cache{InstanceCacheInstanceState::get().request_cache};
  if (auto it{request_cache.find(key)}; it != request_cache.end()) {
    auto cached_instance{from_mixed<InstanceType>(it->second, {})};
    co_return std::move(cached_instance);
  }

  tl::CacheFetch cache_fetch{.key = tl::string{.value = {key.c_str(), key.size()}}};
  tl::storer tls{cache_fetch.footprint()};
  cache_fetch.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::instance_cache::details::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return InstanceType{};
  }

  auto stream{*std::move(expected_stream)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), kphp::component::read_ext::append(response)))) [[unlikely]] {
    co_return InstanceType{};
  }

  tl::fetcher tlf{response};
  tl::Maybe<tl::string> maybe_string{};
  kphp::log::assertion(maybe_string.fetch(tlf));
  if (!maybe_string.opt_value) [[unlikely]] {
    co_return InstanceType{};
  }

  auto cached_instance{f$instance_deserialize<InstanceType>(
      string{(*maybe_string.opt_value).value.data(), static_cast<string::size_type>((*maybe_string.opt_value).value.size())}, {})};
  request_cache.emplace(std::move(key), cached_instance);
  co_return std::move(cached_instance);
}

inline kphp::coro::task<bool> f$instance_cache_update_ttl(string key, int64_t ttl = 0) noexcept {
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("ttl can't be negative: ttl -> {}, key -> {}", ttl, key.c_str());
    co_return false;
  }
  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    kphp::log::warning("ttl exceeds maximum allowed value, key will be stored forever: ttl -> {}, max -> {}, key -> {}", ttl,
                       std::numeric_limits<uint32_t>::max(), key.c_str());
    ttl = 0;
  }

  tl::CacheUpdateTtl cache_update_tll{.key = tl::string{.value = {key.c_str(), key.size()}}, .ttl = tl::u32{.value = static_cast<uint32_t>(ttl)}};
  tl::storer tls{cache_update_tll.footprint()};
  cache_update_tll.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::instance_cache::details::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return false;
  }

  auto stream{*std::move(expected_stream)};
  std::array<std::byte, tl::Bool{}.footprint()> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), response))) [[unlikely]] {
    co_return false;
  }

  tl::Bool tl_bool{};
  tl::fetcher tlf{response};
  kphp::log::assertion(tl_bool.fetch(tlf));
  co_return tl_bool.value;
}

inline kphp::coro::task<bool> f$instance_cache_delete(string key) noexcept {
  InstanceCacheInstanceState::get().request_cache.erase(key);

  tl::CacheDelete cache_delete{.key = tl::string{.value = {key.c_str(), key.size()}}};
  tl::storer tls{cache_delete.footprint()};
  cache_delete.store(tls);

  auto expected_stream{kphp::component::stream::open(kphp::instance_cache::details::COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return false;
  }

  auto stream{*std::move(expected_stream)};
  std::array<std::byte, tl::Bool{}.footprint()> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), response))) [[unlikely]] {
    co_return false;
  }

  tl::Bool tl_bool{};
  tl::fetcher tlf{response};
  kphp::log::assertion(tl_bool.fetch(tlf));
  co_return tl_bool.value;
}
