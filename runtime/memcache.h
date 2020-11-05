// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "runtime/exception.h"
#include "runtime/kphp_core.h"
#include "runtime/memory_usage.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "runtime/rpc.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

void init_memcache_lib();
void free_memcache_lib();

const string mc_prepare_key(const string &key);

mixed mc_get_value(const char *result_str, int32_t result_str_len, int64_t flags);

bool mc_is_immediate_query(const string &key);


constexpr int64_t MEMCACHE_SERIALIZED = 1;
constexpr int64_t MEMCACHE_COMPRESSED = 2;

class C$Memcache : public abstract_refcountable_php_interface {
public:
  virtual void accept(InstanceMemoryEstimateVisitor &) = 0;
  virtual const char *get_class() const = 0;
  virtual int32_t get_hash() const = 0;
};

class C$McMemcache final : public refcountable_polymorphic_php_classes<C$Memcache> {
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

  void accept(InstanceMemoryEstimateVisitor &visitor) final {
    visitor("", hosts);
  }

  const char *get_class() const final {
    return "McMemcache";
  }

  int32_t get_hash() const final {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$McMemcache::get_class())));
  }

  friend inline int32_t f$estimate_memory_usage(const C$McMemcache::host &) {
    return 0;
  }

  array<host> hosts{array_size{1, 0, true}};
};

class C$RpcMemcache final : public refcountable_polymorphic_php_classes<C$Memcache> {
public:
  class host {
  public:
    class_instance<C$RpcConnection> conn;

    host() = default;
    explicit host(class_instance<C$RpcConnection> &&c) :
      conn(std::move(c)) {}
  };

  void accept(InstanceMemoryEstimateVisitor &visitor) final {
    visitor("", hosts);
    visitor("", fake);
  }

  const char *get_class() const final {
    return "RpcMemcache";
  }

  int32_t get_hash() const final {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$RpcMemcache::get_class())));
  }

  friend inline int64_t f$estimate_memory_usage(const C$RpcMemcache::host &h) {
    return f$estimate_memory_usage(h.conn);
  }

  array<host> hosts{array_size{1, 0, true}};
  bool fake{false};
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


class_instance<C$RpcMemcache> f$RpcMemcache$$__construct(const class_instance<C$RpcMemcache> &v$this, bool fake = false);
bool f$RpcMemcache$$addServer(const class_instance<C$RpcMemcache> &v$this, const string &host_name, int64_t port = 11211, bool persistent = true, int64_t weight = 1, double timeout = 1, int64_t retry_interval = 15, bool status = true, const mixed &failure_callback = mixed(), int64_t timeoutms = 0);
bool f$RpcMemcache$$rpc_connect(const class_instance<C$RpcMemcache> &v$this, const string &host_name, int64_t port, const mixed &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);
bool f$RpcMemcache$$add(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
bool f$RpcMemcache$$set(const class_instance<C$RpcMemcache> &v$this,const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
bool f$RpcMemcache$$replace(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &value, int64_t flags = 0, int64_t expire = 0);
mixed f$RpcMemcache$$get(const class_instance<C$RpcMemcache> &v$this, const mixed &key_var);
bool f$RpcMemcache$$delete(const class_instance<C$RpcMemcache> &v$this, const string &key);
mixed f$RpcMemcache$$decrement(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &count = 1);
mixed f$RpcMemcache$$increment(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &count = 1);
mixed f$RpcMemcache$$getVersion(const class_instance<C$RpcMemcache>& v$this);

/*
 *
 *     IMPLEMENTATION
 *
 */


constexpr int32_t MEMCACHE_ERROR = 0x7ae432f5;
constexpr int32_t MEMCACHE_VALUE_NOT_FOUND = 0x32c42422;
constexpr int32_t MEMCACHE_VALUE_LONG = 0x9729c42;
constexpr int32_t MEMCACHE_VALUE_STRING = 0xa6bebb1a;
constexpr int32_t MEMCACHE_FALSE = 0xbc799737;
constexpr int32_t MEMCACHE_TRUE = 0x997275b5;
constexpr int32_t MEMCACHE_SET = 0xeeeb54c4;
constexpr int32_t MEMCACHE_ADD = 0xa358f31c;
constexpr int32_t MEMCACHE_REPLACE = 0x2ecdfaa2;
constexpr int32_t MEMCACHE_INCR = 0x80e6c950;
constexpr int32_t MEMCACHE_DECR = 0x6467e0d9;
constexpr int32_t MEMCACHE_DELETE = 0xab505c0a;
constexpr int32_t MEMCACHE_GET = 0xd33b13ae;

// this is here to avoid full kphp recompilation on any tl scheme change
constexpr int32_t ENGINE_MC_GET_QUERY = 0x62408e9e;

extern const char *mc_method;

class rpc_mc_multiget_resumable : public Resumable {
  using ReturnT = Optional<array<mixed>>;

  int64_t queue_id;
  int64_t first_request_id;
  uint32_t keys_n;
  Optional<int64_t> request_id;
  array<string> query_names;
  array<mixed> result;
  bool return_false_if_not_found;

protected:
  bool run() override {
    RESUMABLE_BEGIN
      while (keys_n > 0) {
        request_id = f$wait_queue_next(queue_id, -1);
        TRY_WAIT(rpc_mc_multiget_resumable_label_0, request_id, decltype(request_id));

        if (request_id.val() <= 0) {
          break;
        }
        keys_n--;

        int64_t k = request_id.val() - first_request_id;
        php_assert (k < query_names.count());

        bool parse_result = rpc_get_and_parse(request_id.val(), -1);
        php_assert (resumable_finished);
        if (!parse_result) {
          continue;
        }

        int32_t op = TRY_CALL_(int32_t, rpc_lookup_int(), RETURN(false));
        if (op == MEMCACHE_ERROR) {
          TRY_CALL_VOID_(rpc_fetch_int(), RETURN(false));//op
          TRY_CALL_VOID_(f$fetch_long(), RETURN(false));//query_id
          TRY_CALL_VOID_(rpc_fetch_int(), RETURN(false)); // error_code
          TRY_CALL_VOID_(f$fetch_string(), RETURN(false)); // error
          if (return_false_if_not_found) {
            result.set_value(query_names.get_value(k), false);
          }
        } else if (op == MEMCACHE_VALUE_NOT_FOUND && !return_false_if_not_found) {
          TRY_CALL_VOID_(rpc_fetch_int(), RETURN(false)); // op
        } else {
          mixed q_result = TRY_CALL_(mixed, f$fetch_memcache_value(), RETURN(false));
          result.set_value(query_names.get_value(k), q_result);
        }

        if (!f$fetch_eof()) {
          php_warning("Not all data fetched during fetch memcache.Value");
        }
      }
      RETURN(result);
    RESUMABLE_END
  }

public:
  rpc_mc_multiget_resumable(int64_t queue_id, int64_t first_request_id, uint32_t keys_n, array<string> query_names, bool return_false_if_not_found) :
    queue_id(queue_id),
    first_request_id(first_request_id),
    keys_n(keys_n),
    request_id(-1),
    query_names(std::move(query_names)),
    result(array_size(0, keys_n, false)),
    return_false_if_not_found(return_false_if_not_found) {
  }
};
