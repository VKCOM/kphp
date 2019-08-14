#pragma once

#include <utility>

#include "runtime/exception.h"
#include "runtime/kphp_core.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "runtime/rpc.h"

void init_memcache_lib();
void free_memcache_lib();

const string mc_prepare_key(const string &key);

var mc_get_value(const char *result_str, int result_str_len, int flags);

bool mc_is_immediate_query(const string &key);


const int MEMCACHE_SERIALIZED = 1;
const int MEMCACHE_COMPRESSED = 2;

class C$Memcache : public abstract_refcountable_php_interface {
};

class C$McMemcache final : public refcountable_polymorphic_php_classes<C$Memcache> {
public:
  class host {
  public:
    int host_num;
    int host_port;
    int host_weight;
    int timeout_ms;

    host();
    host(int host_num, int host_port, int host_weight, int timeout_ms);
  };

  array<host> hosts{array_size{1, 0, true}};
};

class C$RpcMemcache final : public refcountable_polymorphic_php_classes<C$Memcache> {
public:
  class host {
  public:
    rpc_connection conn;
    int host_weight = 0;
    int actor_id = -1;

    host() = default;
    explicit host(const rpc_connection &c): conn(c), host_weight(1) {}
  };

  array<host> hosts{array_size{1, 0, true}};
  bool fake{false};
};

class_instance<C$McMemcache> f$McMemcache$$__construct(const class_instance<C$McMemcache> &v$this);
bool f$McMemcache$$addServer(const class_instance<C$McMemcache> &v$this, const string &host_name, int port = 11211, bool persistent = true, int weight = 1, double timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = 0);
bool f$McMemcache$$add(const class_instance<C$McMemcache> &v$this, const string &key, const var &value, int flags = 0, int expire = 0);
bool f$McMemcache$$set(const class_instance<C$McMemcache> &v$this, const string &key, const var &value, int flags = 0, int expire = 0);
bool f$McMemcache$$replace(const class_instance<C$McMemcache> &v$this, const string &key, const var &value, int flags = 0, int expire = 0);
var f$McMemcache$$get(const class_instance<C$McMemcache> &v$this, const var &key_var);
bool f$McMemcache$$delete(const class_instance<C$McMemcache> &v$this, const string &key);
var f$McMemcache$$decrement(const class_instance<C$McMemcache> &v$this, const string &key, const var &v = 1);
var f$McMemcache$$increment(const class_instance<C$McMemcache> &v$this, const string &key, const var &v = 1);
var f$McMemcache$$getVersion(const class_instance<C$McMemcache> &v$this);
bool f$McMemcache$$rpc_connect(const class_instance<C$McMemcache> &v$this, const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);


class_instance<C$RpcMemcache> f$RpcMemcache$$__construct(const class_instance<C$RpcMemcache> &v$this,bool fake = false);
bool f$RpcMemcache$$addServer(const class_instance<C$RpcMemcache> &v$this, const string &host_name, int port = 11211, bool persistent = true, int weight = 1, double timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = 0);
bool f$RpcMemcache$$rpc_connect(const class_instance<C$RpcMemcache> &v$this, const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);
bool f$RpcMemcache$$add(const class_instance<C$RpcMemcache> &v$this, const string &key, const var &value, int flags = 0, int expire = 0);
bool f$RpcMemcache$$set(const class_instance<C$RpcMemcache> &v$this,const string &key, const var &value, int flags = 0, int expire = 0);
bool f$RpcMemcache$$replace(const class_instance<C$RpcMemcache> &v$this, const string &key, const var &value, int flags = 0, int expire = 0);
var f$RpcMemcache$$get(const class_instance<C$RpcMemcache> &v$this, const var &key_var);
bool f$RpcMemcache$$delete(const class_instance<C$RpcMemcache> &v$this, const string &key);
var f$RpcMemcache$$decrement(const class_instance<C$RpcMemcache> &v$this, const string &key, const var &count = 1);
var f$RpcMemcache$$increment(const class_instance<C$RpcMemcache> &v$this, const string &key, const var &count = 1);
var f$RpcMemcache$$getVersion(const class_instance<C$RpcMemcache>& v$this);



var f$rpc_mc_get(const rpc_connection &conn, const string &key, double timeout = -1.0, bool fake = false);

template<class T>
OrFalse<array<var>> f$rpc_mc_multiget(const rpc_connection &conn, const array<T> &keys, double timeout = -1.0, bool return_false_if_not_found = false, bool run_synchronously = false);

bool f$rpc_mc_set(const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0, bool fake = false);

bool f$rpc_mc_add(const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0, bool fake = false);

bool f$rpc_mc_replace(const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0, bool fake = false);

var f$rpc_mc_increment(const rpc_connection &conn, const string &key, const var &v = 1, double timeout = -1.0, bool fake = false);

var f$rpc_mc_decrement(const rpc_connection &conn, const string &key, const var &v = 1, double timeout = -1.0, bool fake = false);

bool f$rpc_mc_delete(const rpc_connection &conn, const string &key, double timeout = -1.0, bool fake = false);


/*
 *
 *     IMPLEMENTATION
 *
 */


const int MEMCACHE_ERROR = 0x7ae432f5;
const int MEMCACHE_VALUE_NOT_FOUND = 0x32c42422;
const int MEMCACHE_VALUE_LONG = 0x9729c42;
const int MEMCACHE_VALUE_STRING = 0xa6bebb1a;
const int MEMCACHE_FALSE = 0xbc799737;
const int MEMCACHE_TRUE = 0x997275b5;
const int MEMCACHE_SET = 0xeeeb54c4;
const int MEMCACHE_ADD = 0xa358f31c;
const int MEMCACHE_REPLACE = 0x2ecdfaa2;
const int MEMCACHE_INCR = 0x80e6c950;
const int MEMCACHE_DECR = 0x6467e0d9;
const int MEMCACHE_DELETE = 0xab505c0a;
const int MEMCACHE_GET = 0xd33b13ae;

// this is here to avoid full kphp recompilation on any tl scheme change
const int ENGINE_MC_GET_QUERY = 0x62408e9e;

extern const char *mc_method;

class rpc_mc_multiget_resumable : public Resumable {
  using ReturnT = OrFalse<array<var>>;

  int queue_id;
  int first_request_id;
  int keys_n;
  OrFalse<int> request_id;
  array<string> query_names;
  array<var> result;
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

        int k = (int)(request_id.val() - first_request_id);
        php_assert ((unsigned int)k < (unsigned int)query_names.count());

        bool parse_result = f$rpc_get_and_parse(request_id.val(), -1);
        php_assert (resumable_finished);
        if (!parse_result) {
          continue;
        }

        int op = TRY_CALL_(int, rpc_lookup_int(), RETURN(false));
        if (op == MEMCACHE_ERROR) {
          TRY_CALL_VOID_(f$fetch_int(), RETURN(false));//op
          TRY_CALL_VOID_(f$fetch_long(), RETURN(false));//query_id
          TRY_CALL_VOID_(f$fetch_int(), RETURN(false)); // error_code
          TRY_CALL_VOID_(f$fetch_string(), RETURN(false)); // error
          if (return_false_if_not_found) {
            result.set_value(query_names.get_value(k), false);
          }
        } else if (op == MEMCACHE_VALUE_NOT_FOUND && !return_false_if_not_found) {
          TRY_CALL_VOID_(f$fetch_int(), RETURN(false)); // op
        } else {
          var q_result = TRY_CALL_(var, f$fetch_memcache_value(), RETURN(false));
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
  rpc_mc_multiget_resumable(int queue_id, int first_request_id, int keys_n, array<string> query_names, bool return_false_if_not_found) :
    queue_id(queue_id),
    first_request_id(first_request_id),
    keys_n(keys_n),
    request_id(-1),
    query_names(std::move(query_names)),
    result(array_size(0, keys_n, false)),
    return_false_if_not_found(return_false_if_not_found) {
  }
};


template<class T>
OrFalse<array<var>> f$rpc_mc_multiget(const rpc_connection &conn, const array<T> &keys, double timeout, bool return_false_if_not_found, bool run_synchronously, bool fake = false) {
  mc_method = "multiget";
  resumable_finished = true;

  array<string> query_names(array_size(keys.count(), 0, true));
  int queue_id = -1;
  int keys_n = 0;
  int first_request_id = 0;
  int bytes_sent = 0;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    const string key = f$strval(it.get_value());
    const string real_key = mc_prepare_key(key);
    int is_immediate = mc_is_immediate_query(real_key);

    f$rpc_clean();
    f$store_int(fake ? ENGINE_MC_GET_QUERY : MEMCACHE_GET);
    store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

    int current_sent_size = real_key.size() + 32;//estimate
    bytes_sent += current_sent_size;
    if (bytes_sent >= (1 << 15) && bytes_sent > current_sent_size) {
      f$rpc_flush();
      bytes_sent = current_sent_size;
    }
    int request_id = rpc_send(conn, timeout, (bool)is_immediate);
    if (request_id > 0) {
      if (first_request_id == 0) {
        first_request_id = request_id;
      }
      if (!is_immediate) {
        queue_id = wait_queue_push_unsafe(queue_id, request_id);
        keys_n++;
      }
      query_names.push_back(key);
    } else {
      return false;
    }
  }
  if (bytes_sent > 0) {
    f$rpc_flush();
  }

  if (queue_id == -1) {
    return array<var>();
  }

  if (run_synchronously) {
    array<var> result(array_size(0, keys_n, false));

    while (keys_n > 0) {
      int request_id = wait_queue_next_synchronously(queue_id).val();
      if (request_id <= 0) {
        break;
      }
      keys_n--;

      int k = (int)(request_id - first_request_id);
      php_assert ((unsigned int)k < (unsigned int)query_names.count());

      bool parse_result = f$rpc_get_and_parse(request_id, -1);
      php_assert (resumable_finished);
      if (!parse_result) {
        continue;
      }

      int op = TRY_CALL(int, bool, rpc_lookup_int());
      if (op == MEMCACHE_ERROR) {
        TRY_CALL_VOID(bool, f$fetch_int());//op
        TRY_CALL_VOID(bool, f$fetch_long());//query_id
        TRY_CALL_VOID(bool, f$fetch_int());
        TRY_CALL_VOID(bool, f$fetch_string());
        if (return_false_if_not_found) {
          result.set_value(query_names.get_value(k), false);
        }
      } else if (op == MEMCACHE_VALUE_NOT_FOUND && !return_false_if_not_found) {
        TRY_CALL_VOID(bool, f$fetch_int());//op
      } else {
        var q_result = TRY_CALL(var, bool, f$fetch_memcache_value());
        result.set_value(query_names.get_value(k), q_result);
      }

      if (!f$fetch_eof()) {
        php_warning("Not all data fetched during fetch memcache.Value");
      }
    }
    return result;
  }

  return start_resumable<OrFalse<array<var>>>(new rpc_mc_multiget_resumable(queue_id, first_request_id, keys_n, query_names, return_false_if_not_found));
}
