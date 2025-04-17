// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/serialization/msgpack-functions.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

// TODO:
// * Better warnings
// * Better return value
template<typename ClassInstanceType>
kphp::coro::task<bool> f$instance_cache_store(const string& key, const ClassInstanceType& instance, int64_t ttl = 0) noexcept {
  constexpr std::string_view IC_COMPONENT_NAME = "instance_cache";

  tl::TLBuffer buffer;

  if (ttl < 0) [[unlikely]] {
    php_warning("ttl < 0 is noop");
    co_return false;
  }

  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    php_warning("ttl is too big");
    co_return false;
  }

  auto tl_key = tl::string(std::string_view{key.c_str(), key.size()});
  auto serialized_instance = f$instance_serialize(instance);
  if (!serialized_instance.has_value()) {
    php_warning("Cannot serialize instance");
    co_return false;
  }
  auto tl_value = tl::string(std::string_view{serialized_instance.val().c_str(), serialized_instance.val().size()});

  auto tl_cache_store = tl::CacheStore{.key = tl_key, .value = tl_value, .ttl = static_cast<uint32_t>(ttl)};
  tl_cache_store.store(buffer);

  auto query = f$component_client_send_request(string{IC_COMPONENT_NAME.data(), static_cast<string::size_type>(IC_COMPONENT_NAME.size())},
                                               string{buffer.data(), static_cast<string::size_type>(buffer.size())});

  auto _ = co_await f$component_client_fetch_response(co_await query);

  co_return true;
}

template<typename ClassInstanceType>
kphp::coro::task<ClassInstanceType> f$instance_cache_fetch(const string& /*class_name*/, const string& key, bool /*even_if_expired*/ = false) noexcept {
  constexpr std::string_view IC_COMPONENT_NAME = "instance_cache";

  tl::TLBuffer buffer;
  auto tl_key = tl::string(std::string_view{key.c_str(), key.size()});
  auto tl_cache_fetch = tl::CacheFetch{.key = tl_key};
  tl_cache_fetch.store(buffer);

  auto query = f$component_client_send_request(string{IC_COMPONENT_NAME.data(), static_cast<string::size_type>(IC_COMPONENT_NAME.size())},
                                               string{buffer.data(), static_cast<string::size_type>(buffer.size())});

  auto resp = co_await f$component_client_fetch_response(co_await query);
  buffer.clean();
  buffer.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::Maybe<tl::string> tl_resp;
  if (!tl_resp.fetch(buffer)) {
    php_notice("Wrong response for key \"%s\"", key.c_str());
    co_return ClassInstanceType{};
  }
  if (!tl_resp.opt_value.has_value()) {
    php_notice("Key \"%s\" is not in cache", key.c_str());
    co_return ClassInstanceType{};
  }
  co_return f$instance_deserialize<ClassInstanceType>(string(tl_resp.opt_value->value.data(), tl_resp.opt_value->value.size()), {});
}

inline kphp::coro::task<bool> f$instance_cache_update_ttl(const string& key, int64_t ttl = 0) noexcept {
  constexpr std::string_view IC_COMPONENT_NAME = "instance_cache";
  if (ttl < 0) [[unlikely]] {
    php_warning("ttl < 0 is noop");
    co_return false;
  }

  if (ttl > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    php_warning("ttl is too big");
    co_return false;
  }
  tl::TLBuffer buffer;
  auto tl_key = tl::string(std::string_view{key.c_str(), key.size()});
  auto tl_update_ttl = tl::CacheUpdateTtl{.key = tl_key, .ttl = static_cast<uint32_t>(ttl)};
  tl_update_ttl.store(buffer);

  auto query = f$component_client_send_request(string{IC_COMPONENT_NAME.data(), static_cast<string::size_type>(IC_COMPONENT_NAME.size())},
                                               string{buffer.data(), static_cast<string::size_type>(buffer.size())});

  auto resp = co_await f$component_client_fetch_response(co_await query);
  // TODO change api to better return value

  co_return true;
}

inline kphp::coro::task<bool> f$instance_cache_delete(const string& key) noexcept {
  constexpr std::string_view IC_COMPONENT_NAME = "instance_cache";
  tl::TLBuffer buffer;
  auto tl_key = tl::string(std::string_view{key.c_str(), key.size()});
  auto tl_cache_delete = tl::CacheDelete{.key = tl_key};
  tl_cache_delete.store(buffer);

  auto query = f$component_client_send_request(string{IC_COMPONENT_NAME.data(), static_cast<string::size_type>(IC_COMPONENT_NAME.size())},
                                               string{buffer.data(), static_cast<string::size_type>(buffer.size())});

  auto resp = co_await f$component_client_fetch_response(co_await query);
  // TODO change api to better return value

  co_return true;
}
