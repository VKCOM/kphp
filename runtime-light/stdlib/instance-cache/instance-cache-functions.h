// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/logs.h"

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

  tl::TLBuffer tlb;
  tl::CacheStore{.key = tl::string{.value = {key.c_str(), key.size()}},
                 .value = tl::string{.value = {serialized_instance.val().c_str(), serialized_instance.val().size()}},
                 .ttl = tl::u32{.value = static_cast<uint32_t>(ttl)}}
      .store(tlb);

  auto query{co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (query.is_null()) [[unlikely]] {
    co_return false;
  }

  auto response{co_await f$component_client_fetch_response(std::move(query))};
  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  tl::Bool tl_bool{};
  kphp::log::assertion(tl_bool.fetch(tlb));
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

  tl::TLBuffer tlb;
  tl::CacheFetch{.key = tl::string{.value = {key.c_str(), key.size()}}}.store(tlb);

  auto query{co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (query.is_null()) [[unlikely]] {
    co_return InstanceType{};
  }

  auto response{co_await f$component_client_fetch_response(std::move(query))};
  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  tl::Maybe<tl::string> maybe_string{};
  kphp::log::assertion(maybe_string.fetch(tlb));
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

  tl::TLBuffer tlb;
  tl::CacheUpdateTtl{.key = tl::string{.value = {key.c_str(), key.size()}}, .ttl = tl::u32{.value = static_cast<uint32_t>(ttl)}}.store(tlb);

  auto query{co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (query.is_null()) [[unlikely]] {
    co_return false;
  }

  auto response{co_await f$component_client_fetch_response(std::move(query))};
  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  tl::Bool tl_bool{};
  kphp::log::assertion(tl_bool.fetch(tlb));
  co_return tl_bool.value;
}

inline kphp::coro::task<bool> f$instance_cache_delete(string key) noexcept {
  InstanceCacheInstanceState::get().request_cache.erase(key);

  tl::TLBuffer tlb;
  tl::CacheDelete{.key = tl::string{.value = {key.c_str(), key.size()}}}.store(tlb);

  auto query{co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (query.is_null()) [[unlikely]] {
    co_return false;
  }

  auto response{co_await f$component_client_fetch_response(std::move(query))};
  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  tl::Bool tl_bool{};
  kphp::log::assertion(tl_bool.fetch(tlb));
  co_return tl_bool.value;
}
