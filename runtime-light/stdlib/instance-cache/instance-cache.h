// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>

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
constexpr std::string_view COMPONENT_NAME = "instance_cache";
} // namespace kphp::instance_cache::details

template<typename ClassInstanceType>
kphp::coro::task<bool> f$instance_cache_store(string key, ClassInstanceType instance, int64_t ttl = 0) noexcept {
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("ttl < 0 is noop (key: {})", key.c_str());
    co_return false;
  }

  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    kphp::log::warning("ttl is too big, will store forever (key: {})", key.c_str());
    ttl = 0;
  }

  auto serialized_instance = f$instance_serialize(instance);
  if (!serialized_instance.has_value()) [[unlikely]] {
    kphp::log::warning("cannot serialize instance (key: {})", key.c_str());
    co_return false;
  }

  tl::TLBuffer tlb;

  tl::CacheStore{.key = tl::string(std::string_view{key.c_str(), key.size()}),
                 .value = tl::string(std::string_view{serialized_instance.val().c_str(), serialized_instance.val().size()}),
                 .ttl = tl::u32{static_cast<uint32_t>(ttl)}}
      .store(tlb);

  auto query = co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())});

  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("cannot send instance_cache_store() request to platform (key: {})", key.c_str());
    co_return false;
  }

  auto resp = co_await f$component_client_fetch_response(query);
  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});
  tl::Bool tl_resp;
  kphp::log::assertion(tl_resp.fetch(tlb));
  InstanceCacheInstanceState::get().request_cache.store(key, instance);
  co_return tl_resp.value;
}

template<typename ClassInstanceType>
kphp::coro::task<ClassInstanceType> f$instance_cache_fetch(string /*class_name*/, string key, bool /*even_if_expired*/ = false) noexcept {
  auto& request_cache = InstanceCacheInstanceState::get().request_cache;
  if (auto cached = request_cache.fetch<ClassInstanceType>(key); !cached.is_null()) {
    co_return std::move(cached);
  }

  tl::TLBuffer tlb;
  tl::CacheFetch{.key = tl::string(std::string_view{key.c_str(), key.size()})}.store(tlb);

  auto query = co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())});

  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("cannot send instance_cache_fetch() request to platform (key: {})", key.c_str());
    co_return ClassInstanceType{};
  }

  auto resp = co_await f$component_client_fetch_response(query);
  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::Maybe<tl::string> tl_resp;
  kphp::log::assertion(tl_resp.fetch(tlb));
  if (!tl_resp.opt_value.has_value()) [[unlikely]] {
    co_return ClassInstanceType{};
  }

  auto response = f$instance_deserialize<ClassInstanceType>(string(tl_resp.opt_value->value.data(), tl_resp.opt_value->value.size()), {});
  request_cache.store(key, response);
  co_return std::move(response);
}

inline kphp::coro::task<bool> f$instance_cache_update_ttl(string key, int64_t ttl = 0) noexcept {
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("ttl < 0 is noop (key: {})", key.c_str());
    co_return false;
  }

  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    kphp::log::warning("ttl is too big, will store forever (key: {})", key.c_str());
    ttl = 0;
  }
  tl::TLBuffer tlb;
  tl::CacheUpdateTtl{.key = tl::string(std::string_view{key.c_str(), key.size()}), .ttl = tl::u32{static_cast<uint32_t>(ttl)}}.store(tlb);

  auto query = co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())});

  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("cannot send instance_cache_update_ttl() request to platform (key: {})", key.c_str());
    co_return false;
  }

  auto resp = co_await f$component_client_fetch_response(query);
  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});
  tl::Bool tl_resp;
  kphp::log::assertion(tl_resp.fetch(tlb));

  co_return tl_resp.value;
}

inline kphp::coro::task<bool> f$instance_cache_delete(string key) noexcept {
  InstanceCacheInstanceState::get().request_cache.del(key);

  tl::TLBuffer tlb;
  tl::CacheDelete{.key = tl::string(std::string_view{key.c_str(), key.size()})}.store(tlb);

  auto query = co_await f$component_client_send_request(
      string{kphp::instance_cache::details::COMPONENT_NAME.data(), static_cast<string::size_type>(kphp::instance_cache::details::COMPONENT_NAME.size())},
      string{tlb.data(), static_cast<string::size_type>(tlb.size())});

  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("cannot send instance_cache_update_ttl() request to platform (key: {})", key.c_str());
    co_return false;
  }

  auto resp = co_await f$component_client_fetch_response(query);
  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});
  tl::Bool tl_resp;
  kphp::log::assertion(tl_resp.fetch(tlb));

  co_return tl_resp.value;
}
