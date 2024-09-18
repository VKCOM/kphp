// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-core/runtime-core.h"
#include "runtime/dummy-visitor-methods.h"
#include "runtime/memory_usage.h"

void init_memcache_lib();
void free_memcache_lib();

const string mc_prepare_key(const string &key);

mixed mc_get_value(const char *result_str, int32_t result_str_len, int64_t flags);

bool mc_is_immediate_query(const string &key);

inline constexpr int64_t MEMCACHE_SERIALIZED = 1U << 0U;
inline constexpr int64_t MEMCACHE_COMPRESSED = 1U << 1U;
inline constexpr int64_t MEMCACHE_JSON_SERIALZIED = 1U << 4U;

struct C$Memcache : public abstract_refcountable_php_interface {
public:
  virtual void accept(CommonMemoryEstimateVisitor &) = 0;
  virtual const char *get_class() const = 0;
  virtual int32_t get_hash() const = 0;
};

class C$McMemcache final : public refcountable_polymorphic_php_classes<C$Memcache>, private DummyVisitorMethods {
public:
  class host {
  public:
    int32_t host_num;
    int32_t host_port;
    int32_t host_weight;
    int32_t timeout_ms;

    host();
    host(int32_t host_num, int32_t host_port, int32_t host_weight, int32_t timeout_ms);
  };

  using DummyVisitorMethods::accept;

  void accept(CommonMemoryEstimateVisitor &visitor [[maybe_unused]]) final {
    visitor("", array<int64_t>{});
  }

  const char *get_class() const final {
    return "McMemcache";
  }

  int32_t get_hash() const final {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$McMemcache::get_class())));
  }

  virtual C$McMemcache* virtual_builtin_clone() const noexcept {
    return new C$McMemcache{*this};
  }

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return sizeof(*this);
  }

  array<host> hosts{array_size{1, true}};
};

class_instance<C$McMemcache> f$McMemcache$$__construct(const class_instance<C$McMemcache> &v$this);
bool f$McMemcache$$addServer(const class_instance<C$McMemcache> &v$this, const string &host_name, int64_t port = 11211,
                             bool persistent = true, int64_t weight = 1, double timeout = 1, int64_t retry_interval = 15,
                             bool status = true, const mixed &failure_callback = mixed(), int64_t timeoutms = 0);
bool f$McMemcache$$add(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
bool f$McMemcache$$set(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
bool f$McMemcache$$replace(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
mixed f$McMemcache$$get(const class_instance<C$McMemcache> &v$this, const mixed &key_var);
bool f$McMemcache$$delete(const class_instance<C$McMemcache> &v$this, const string &key);
mixed f$McMemcache$$decrement(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &v = 1);
mixed f$McMemcache$$increment(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &v = 1);
mixed f$McMemcache$$getVersion(const class_instance<C$McMemcache> &v$this);
bool f$McMemcache$$rpc_connect(const class_instance<C$McMemcache> &v$this, const string &host_name, int64_t port, const mixed &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

extern const char *mc_method;
