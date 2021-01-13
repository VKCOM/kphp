// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memcache.h"

#include <cstdlib>
#include <ctime>

#include "common/tl/constants/engine.h"
#include "common/wrappers/gnu-builtins.h"

static_assert(TL_ENGINE_MC_GET_QUERY == ENGINE_MC_GET_QUERY, "bad ENGINE_MC_GET_QUERY constant");


#include "runtime/array_functions.h"
#include "runtime/datetime.h"
#include "runtime/interface.h"
#include "runtime/math_functions.h"
#include "runtime/misc.h"
#include "runtime/openssl.h"
#include "runtime/regexp.h"
#include "runtime/string_functions.h"
#include "runtime/zlib.h"
#include "server/php-queries.h"

static string_buffer drivers_SB(1024);

constexpr string::size_type MAX_KEY_LEN = 1000;
constexpr string::size_type MAX_VALUE_LEN = (1 << 20);
const int MAX_INPUT_VALUE_LEN = (1 << 24);

const string UNDERSCORE("_", 1);


const char *mc_method{nullptr};
static const char *mc_last_key{nullptr};
static int mc_last_key_len{0};
static mixed mc_res;
static bool mc_bool_res{false};

mixed rpc_mc_run_increment(int op, const class_instance<C$RpcConnection> &conn, const string &key, int64_t v, double timeout);
bool rpc_mc_run_set(int32_t op, const class_instance<C$RpcConnection> &conn, const string &key, const mixed &value, int64_t flags, int64_t expire, double timeout);
bool f$rpc_mc_delete(const class_instance<C$RpcConnection> &conn, const string &key, double timeout = -1.0, bool fake = false);
mixed f$rpc_mc_get(const class_instance<C$RpcConnection> &conn, const string &key, double timeout = -1.0, bool fake = false);

Optional<array<mixed>> f$rpc_mc_multiget(const class_instance<C$RpcConnection> &conn, const array<mixed> &keys, double timeout, bool fake = false) {
  mc_method = "multiget";
  resumable_finished = true;

  array<string> query_names(array_size(keys.count(), 0, true));
  int64_t queue_id = -1;
  uint32_t keys_n = 0;
  int64_t first_request_id = 0;
  size_t bytes_sent = 0;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    const string key = f$strval(it.get_value());
    const string real_key = mc_prepare_key(key);
    const bool is_immediate = mc_is_immediate_query(real_key);

    f$rpc_clean();
    store_int(fake ? ENGINE_MC_GET_QUERY : MEMCACHE_GET);
    store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

    size_t current_sent_size = real_key.size() + 32;//estimate
    bytes_sent += current_sent_size;
    if (bytes_sent >= (1 << 15) && bytes_sent > current_sent_size) {
      f$rpc_flush();
      bytes_sent = current_sent_size;
    }
    int64_t request_id = rpc_send(conn, timeout, is_immediate);
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
    return array<mixed>();
  }

  array<mixed> result(array_size(0, keys_n, false));

  while (keys_n > 0) {
    int64_t request_id = wait_queue_next_synchronously(queue_id).val();
    if (request_id <= 0) {
      break;
    }
    keys_n--;

    int64_t k = request_id - first_request_id;
    php_assert(static_cast<uint64_t>(k) < query_names.count());

    bool parse_result = rpc_get_and_parse(request_id, -1);
    php_assert (resumable_finished);
    if (!parse_result) {
      continue;
    }

    int32_t op = TRY_CALL(int32_t, bool, rpc_lookup_int());
    if (op == MEMCACHE_ERROR) {
      TRY_CALL_VOID(bool, rpc_fetch_int());//op
      TRY_CALL_VOID(bool, f$fetch_long());//query_id
      TRY_CALL_VOID(bool, rpc_fetch_int());
      TRY_CALL_VOID(bool, f$fetch_string());
    } else if (op == MEMCACHE_VALUE_NOT_FOUND) {
      TRY_CALL_VOID(bool, rpc_fetch_int());//op
    } else {
      mixed q_result = TRY_CALL(mixed, bool, f$fetch_memcache_value());
      result.set_value(query_names.get_value(k), q_result);
    }

    if (!f$fetch_eof()) {
      php_warning("Not all data fetched during fetch memcache.Value");
    }
  }
  return result;
}

const string mc_prepare_key(const string &key) {
  if (key.size() < 3) {
    php_warning("Very short key \"%s\" in Memcache::%s", key.c_str(), mc_method);
  }

  bool bad_key = (key.size() > MAX_KEY_LEN || key.empty());
  for (int i = 0; i < (int)key.size() && !bad_key; i++) {
    if ((unsigned int)key[i] <= 32u) {
      bad_key = true;
    }
  }
  if (!bad_key) {
    return key;
  }

  string real_key = key.substr(0, min(MAX_KEY_LEN, key.size()));//need a copy
  for (int i = 0; i < (int)real_key.size(); i++) {
    if ((unsigned int)real_key[i] <= 32u) {
      real_key[i] = '_';
    }
  }
  if (real_key.empty()) {
    php_warning("Empty parameter key in Memcache::%s, key \"_\" used instead", mc_method);
    real_key = UNDERSCORE;
  } else {
    php_warning("Wrong parameter key = \"%s\" in Memcache::%s, key \"%s\" used instead", key.c_str(), mc_method, real_key.c_str());
  }
  return real_key;
}


bool mc_is_immediate_query(const string &key) {
  return key[0] == '^' && key.size() >= 2;
}

const char *mc_parse_value(const char *result, int result_len, const char **key, int *key_len, const char **value, int *value_len, int *flags, int *error_code) {
  if (strncmp(result, "VALUE", 5)) {
    *error_code = 1;
    return nullptr;
  }
  int i = 5;

  while (result[i] == ' ') {
    i++;
  }
  if (i == result_len) {
    *error_code = 2;
    return nullptr;
  }
  *key = result + i;
  while (i < result_len && result[i] != ' ') {
    i++;
  }
  if (i == result_len) {
    *error_code = 3;
    return nullptr;
  }
  *key_len = (int)(result + i - *key);

  while (result[i] == ' ') {
    i++;
  }
  if (i == result_len) {
    *error_code = 4;
    return nullptr;
  }
  *value = result + i;
  while (i < result_len && result[i] != ' ') {
    i++;
  }
  if (i == result_len) {
    *error_code = 5;
    return nullptr;
  }
  int64_t flags64 = 0;
  if (!php_try_to_int(*value, static_cast<size_t>(result + i - *value), &flags64)) {
    *error_code = 6;
    return nullptr;
  }
  if (int64_t{static_cast<int32_t>(flags64)} != flags64) {
    *error_code = 6;
    return nullptr;
  }
  *flags = static_cast<int32_t>(flags64);

  while (result[i] == ' ') {
    i++;
  }
  if (i == result_len) {
    *error_code = 7;
    return nullptr;
  }
  *value = result + i;
  while (i < result_len && result[i] != ' ' && result[i] != '\r') {
    i++;
  }
  if (result[i] != '\r' || result[i + 1] != '\n') {
    *error_code = 8;
    return nullptr;
  }
  int64_t value_len64 = 0;
  if (!php_try_to_int(*value, static_cast<size_t>(result + i - *value), &value_len64)
      || value_len64 < 0
      || value_len64 >= MAX_INPUT_VALUE_LEN) {
    *error_code = 9;
    return nullptr;
  }
  *value_len = static_cast<int32_t>(value_len64);

  i += 2;
  *value = result + i;
  if (i + *value_len + 2 > result_len) {
    *error_code = 10;
    return nullptr;
  }
  i += *value_len;

  if (result[i] != '\r' || result[i + 1] != '\n') {
    *error_code = 11;
    return nullptr;
  }
  *error_code = 0;
  return result + i + 2;
}

mixed mc_get_value(const char *result_str, int32_t result_str_len, int64_t flags) {
  mixed result;
  if (flags & MEMCACHE_COMPRESSED) {
    flags ^= MEMCACHE_COMPRESSED;
    string::size_type uncompressed_len;
    result_str = gzuncompress_raw({result_str, static_cast<size_t>(result_str_len)}, &uncompressed_len);
    result_str_len = uncompressed_len;
  }

  if (flags & MEMCACHE_SERIALIZED) {
    flags ^= MEMCACHE_SERIALIZED;
    result = unserialize_raw(result_str, result_str_len);
  } else {
    result = string(result_str, result_str_len);
  }

  return result;
}

void mc_set_callback(const char *result, int result_len __attribute__((unused))) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }

  if (!strcmp(result, "STORED\r\n")) {
    mc_bool_res = true;
    return;
  }
  if (!strcmp(result, "NOT_STORED\r\n")) {
    mc_bool_res = false;
    return;
  }

  php_warning("Strange result \"%s\" returned from memcached in Memcache::%s with key %s", result, mc_method, mc_last_key);
  mc_bool_res = false;
}

void mc_multiget_callback(const char *result, int result_len) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }
  const char *full_result = result;

  while (*result) {
    switch (*result) {
      case 'V': {
        int key_len, value_len;
        const char *key, *value;
        int flags;
        int error_code;

        const char *new_result = mc_parse_value(result, result_len, &key, &key_len, &value, &value_len, &flags, &error_code);
        if (!new_result) {
          php_warning("Wrong memcache response \"%s\" in Memcache::get with multikey %s and error code %d", result, mc_last_key, error_code);
          return;
        }
        php_assert (new_result > result);
        result_len -= (int)(new_result - result);
        php_assert (result_len >= 0);
        result = new_result;
        mc_res.set_value(string(key, key_len), mc_get_value(value, value_len, flags));
        break;
      }
      case 'E':
        if (result_len == 5 && !strncmp(result, "END\r\n", 5)) {
          return;
        }
        /* fallthrough */
      default:
        php_warning("Wrong memcache response \"%s\" in Memcache::get with multikey %s", full_result, mc_last_key);
    }
  }
}

void mc_get_callback(const char *result, int result_len) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }
  const char *full_result = result;

  switch (*result) {
    case 'V': {
      int key_len, value_len;
      const char *key, *value;
      int flags;
      int error_code;

      const char *new_result = mc_parse_value(result, result_len, &key, &key_len, &value, &value_len, &flags, &error_code);
      if (!new_result) {
        php_warning("Wrong memcache response \"%s\" in Memcache::get with key %s and error code %d", result, mc_last_key, error_code);
        return;
      }
      if (mc_last_key_len != key_len || memcmp(mc_last_key, key, (size_t)key_len)) {
        php_warning("Wrong memcache response \"%s\" in Memcache::get with key %s", result, mc_last_key);
        return;
      }
      php_assert (new_result > result);
      result_len -= (int)(new_result - result);
      php_assert (result_len >= 0);
      result = new_result;
      mc_res = mc_get_value(value, value_len, flags);
    }
      /* fallthrough */
    case 'E':
      if (result_len == 5 && !strncmp(result, "END\r\n", 5)) {
        return;
      }
      /* fallthrough */
    default:
      php_warning("Wrong memcache response \"%s\" in Memcache::get with key %s", full_result, mc_last_key);
  }
}

void mc_delete_callback(const char *result, int result_len __attribute__((unused))) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }

  if (!strcmp(result, "NOT_FOUND\r\n")) {
    mc_bool_res = false;
  } else if (!strcmp(result, "DELETED\r\n")) {
    mc_bool_res = true;
  } else {
    php_warning("Strange result \"%s\" returned from memcached in Memcache::delete with key %s", result, mc_last_key);
    mc_bool_res = false;
  }
}

void mc_increment_callback(const char *result, int result_len) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }

  if (!strcmp(result, "NOT_FOUND\r\n")) {
    mc_res = false;
  } else {
    if (result_len >= 2 && result[result_len - 2] == '\r' && result[result_len - 1] == '\n') {
      mc_res.assign(result, result_len - 2);
    } else {
      php_warning("Wrong memcache response \"%s\" in Memcache::%sement with key %s", result, mc_method, mc_last_key);
    }
  }
}

void mc_version_callback(const char *result, int result_len) {
  if (!strcmp(result, "ERROR\r\n")) {
    return;
  }

  switch (*result) {
    case 'V': {
      if (!strncmp(result, "VERSION ", 8)) {
        if (result_len >= 10 && result[result_len - 2] == '\r' && result[result_len - 1] == '\n') {
          mc_res.assign(result + 8, result_len - 10);
          break;
        }
      }
    }
      /* fallthrough */
    default:
      php_warning("Wrong memcache response \"%s\" in Memcache::getVersion", result);
  }
}


C$McMemcache::host::host() :
  host_num(-1),
  host_port(-1),
  host_weight(0),
  timeout_ms(200) {
}

C$McMemcache::host::host(int32_t host_num, int32_t host_port, int32_t host_weight, int32_t timeout_ms) :
  host_num(host_num),
  host_port(host_port),
  host_weight(host_weight),
  timeout_ms(timeout_ms) {
}


C$McMemcache::host get_host(const array<C$McMemcache::host> &hosts) {
  php_assert (hosts.count() > 0);

  return hosts.get_value(f$array_rand(hosts));
}


static bool run_set(const class_instance<C$McMemcache> &mc, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  if (mc->hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::%s with key \"%s\"", mc_method, key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);

  if (flags & ~MEMCACHE_COMPRESSED) {
    php_warning("Wrong parameter flags = %" PRIi64 " in Memcache::%s", flags, mc_method);
    flags &= MEMCACHE_COMPRESSED;
  }

  if ((unsigned int)expire > (unsigned int)(30 * 24 * 60 * 60)) {
    php_warning("Wrong parameter expire = %" PRIi64 " in Memcache::%s", expire, mc_method);
    expire = 0;
  }

  string string_value;
  if (f$is_array(value)) {
    string_value = f$serialize(value);
    flags |= MEMCACHE_SERIALIZED;
  } else {
    string_value = value.to_string();
  }

  if (flags & MEMCACHE_COMPRESSED) {
    string_value = f$gzcompress(string_value);
  }

  if (string_value.size() >= MAX_VALUE_LEN) {
    php_warning("Parameter value has length %d and too large for storing in Memcache", (int)string_value.size());
    return false;
  }

  drivers_SB.clean() << mc_method
                     << ' ' << real_key
                     << ' ' << flags
                     << ' ' << expire
                     << ' ' << (int)string_value.size()
                     << "\r\n" << string_value
                     << "\r\n";

  mc_bool_res = false;
  auto cur_host = get_host(mc->hosts);
  if (mc_is_immediate_query(real_key)) {
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr);
    return true;
  } else {
    mc_last_key = real_key.c_str();
    mc_last_key_len = (int)real_key.size();
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_set_callback);
    return mc_bool_res;
  }
}

mixed run_increment(const class_instance<C$McMemcache> &mc, const string &key, const mixed &count) {
  if (mc->hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::%s with key \"%s\"", mc_method, key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);

  drivers_SB.clean() << mc_method << ' ' << real_key << ' ';

  if (count.is_int()) {
    drivers_SB << count;
  } else {
    string count_str = count.to_string();

    int i, negative = (count_str[0] == '-'), len = count_str.size();
    for (i = negative; '0' <= count_str[i] && count_str[i] <= '9'; i++) {
    }

    if (i < len || len == negative || len > 19 + negative) {
      php_warning("Wrong parameter count = \"%s\" in Memcache::%sement, key %semented by 1 instead", count_str.c_str(), mc_method, mc_method);
      drivers_SB << '1';
    } else {
      drivers_SB << count_str;
    }
  }
  drivers_SB << "\r\n";

  mc_res = false;
  auto cur_host = get_host(mc->hosts);
  if (mc_is_immediate_query(real_key)) {
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr);
    return 0;
  } else {
    mc_last_key = real_key.c_str();
    mc_last_key_len = (int)real_key.size();
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_increment_callback);
    return mc_res;
  }
}

bool f$McMemcache$$addServer(const class_instance<C$McMemcache> & mc, const string &host_name, int64_t port,
                             bool persistent __attribute__((unused)), int64_t weight, double timeout, int64_t retry_interval __attribute__((unused)),
                             bool status __attribute__((unused)), const mixed &failure_callback __attribute__((unused)), int64_t timeoutms) {
  int32_t result_timeout = static_cast<int32_t>(static_cast<int64_t>(timeout * 1000) + timeoutms);

  if (result_timeout <= 0) {
    result_timeout = 1;
  }
  if (result_timeout >= MAX_TIMEOUT) {
    result_timeout = MAX_TIMEOUT;
  }

  int host_num = mc_connect_to(host_name.c_str(), static_cast<int32_t>(port));
  if (host_num >= 0) {
    mc->hosts.push_back({host_num, static_cast<int32_t>(port), static_cast<int32_t>(weight), result_timeout});
  }
  return host_num >= 0;
}

bool f$RpcMemcache$$addServer(const class_instance<C$RpcMemcache> &, const string &, int64_t, bool, int64_t, double, int64_t, bool, const mixed &, int64_t) {
  php_warning("addServer used on rpc-Memcache object");
  return false;
}

bool f$McMemcache$$add(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  mc_method = "add";
  return run_set(v$this, key, value, flags, expire);
}

bool f$McMemcache$$set(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  mc_method = "set";
  return run_set(v$this, key, value, flags, expire);
}

bool f$McMemcache$$replace(const class_instance<C$McMemcache> &v$this, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  mc_method = "replace";
  return run_set(v$this, key, value, flags, expire);
}

mixed f$McMemcache$$get(const class_instance<C$McMemcache> &v$this, const mixed &key_var) {
  mc_method = "get";
  if (f$is_array(key_var)) {
    if (v$this->hosts.count() <= 0) {
      php_warning("There is no available server to run Memcache::get");
      return array<mixed>();
    }

    drivers_SB.clean();
    drivers_SB << "get";
    bool is_immediate_query = true;
    for (array<mixed>::const_iterator p = key_var.begin(); p != key_var.end(); ++p) {
      const string key = p.get_value().to_string();
      const string real_key = mc_prepare_key(key);
      drivers_SB << ' ' << real_key;
      is_immediate_query = is_immediate_query && mc_is_immediate_query(real_key);
    }
    drivers_SB << "\r\n";

    mc_res = array<mixed>(array_size(0, key_var.count(), false));
    auto cur_host = get_host(v$this->hosts);
    if (is_immediate_query) {
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr); //TODO wrong if we have no mc_proxy
    } else {
      mc_last_key = drivers_SB.c_str();
      mc_last_key_len = (int)drivers_SB.size();
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_multiget_callback); //TODO wrong if we have no mc_proxy
    }
  } else {
    if (v$this->hosts.count() <= 0) {
      php_warning("There is no available server to run Memcache::get with key \"%s\"", key_var.to_string().c_str());
      return false;
    }

    const string key = key_var.to_string();
    const string real_key = mc_prepare_key(key);

    drivers_SB.clean() << "get " << real_key << "\r\n";

    auto cur_host = get_host(v$this->hosts);
    if (mc_is_immediate_query(real_key)) {
      mc_res = true;
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr);
    } else {
      mc_res = false;
      mc_last_key = real_key.c_str();
      mc_last_key_len = (int)real_key.size();
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_get_callback);
    }
  }
  return mc_res;
}

bool f$McMemcache$$delete(const class_instance<C$McMemcache> &v$this,const string &key) {
  mc_method = "delete";
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::delete with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);

  drivers_SB.clean() << "delete " << real_key << "\r\n";

  mc_bool_res = false;
  auto cur_host = get_host(v$this->hosts);
  if (mc_is_immediate_query(real_key)) {
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr);
    return true;
  } else {
    mc_last_key = real_key.c_str();
    mc_last_key_len = (int)real_key.size();
    mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_delete_callback);
    return mc_bool_res;
  }
}

mixed f$McMemcache$$decrement(const class_instance<C$McMemcache> &mc, const string &key, const mixed &count) {
  mc_method = "decr";
  return run_increment(mc, key, count);
}

mixed f$McMemcache$$increment(const class_instance<C$McMemcache> &mc,const string &key, const mixed &count) {
  mc_method = "incr";
  return run_increment(mc, key, count);
}

mixed f$McMemcache$$getVersion(const class_instance<C$McMemcache>& v$this) {
  static const char *version_str = "version\r\n";
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::getVersion");
    return false;
  }

  mc_res = false;
  auto cur_host = get_host(v$this->hosts);
  mc_run_query(cur_host.host_num, version_str, (int)strlen(version_str), cur_host.timeout_ms, 1, mc_version_callback);

  return mc_res;
}


static C$RpcMemcache::host get_host(const array<C$RpcMemcache::host> &hosts) {
  php_assert (hosts.count() > 0);
  return hosts.get_value(f$array_rand(hosts));
}

bool f$RpcMemcache$$rpc_connect(const class_instance<C$RpcMemcache> &v$this, const string &host_name, int64_t port, const mixed &default_actor_id, double timeout, double connect_timeout, double reconnect_timeout) {
  class_instance<C$RpcConnection> c = f$new_rpc_connection(host_name, port, default_actor_id, timeout, connect_timeout, reconnect_timeout);
  if (!c.is_null() && c.get()->host_num >= 0) {
    auto h = C$RpcMemcache::host(std::move(c));
    v$this->hosts.push_back(std::move(h));
    return true;
  }
  return false;
}

bool f$McMemcache$$rpc_connect(const class_instance<C$McMemcache> &, const string &, int64_t, const mixed &, double, double, double) {
  php_warning("rpc_connect used on non-rpc Memcache object");
  return false;
}

template <typename T1, typename T2, typename RetT = T1>
RetT catchException(T1 result, T2 def, const char *function = __builtin_FUNCTION()) {
  if (!CurException.is_null()) {
    class_instance<C$Throwable> e = std::move(CurException);
    php_warning("Exception caught in %s: %s", function, e->$message.c_str());
    return def;
  }
  return result;
}

bool f$RpcMemcache$$add(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::add with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(v$this->hosts);
  mc_method = "add";
  return catchException(rpc_mc_run_set(v$this->fake ? TL_ENGINE_MC_ADD_QUERY : MEMCACHE_ADD, cur_host.conn, real_key, value, flags, expire, -1), false);
}

bool f$RpcMemcache$$set(const class_instance<C$RpcMemcache> &v$this,const string &key, const mixed &value, int64_t flags, int64_t expire) {
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::set with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(v$this->hosts);
  mc_method = "set";
  return catchException(rpc_mc_run_set(v$this->fake ? TL_ENGINE_MC_SET_QUERY : MEMCACHE_SET, cur_host.conn, real_key, value, flags, expire, -1), false);
}

bool f$RpcMemcache$$replace(const class_instance<C$RpcMemcache> &v$this, const string &key, const mixed &value, int64_t flags, int64_t expire) {
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::replace with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(v$this->hosts);
  mc_method = "replace";
  return catchException(rpc_mc_run_set(v$this->fake ? TL_ENGINE_MC_REPLACE_QUERY : MEMCACHE_REPLACE, cur_host.conn, real_key, value, flags, expire, -1.0), false);
}

mixed f$RpcMemcache$$get(const class_instance<C$RpcMemcache> &mc, const mixed &key_var) {
  if (f$is_array(key_var)) {
    if (mc->hosts.count() <= 0) {
      php_warning("There is no available server to run RpcMemcache::get");
      return array<mixed>();
    }

    auto cur_host = get_host(mc->hosts);
    mixed res = f$rpc_mc_multiget(cur_host.conn, key_var.to_array(), -1.0, mc->fake);
    php_assert(resumable_finished);
    return catchException<mixed>(res, array<mixed>());
  } else {
    if (mc->hosts.count() <= 0) {
      php_warning("There is no available server to run RpcMemcache::get with key \"%s\"", key_var.to_string().c_str());
      return false;
    }

    const string key = key_var.to_string();
    const string real_key = mc_prepare_key(key);

    auto cur_host = get_host(mc->hosts);
    return catchException<mixed>(f$rpc_mc_get(cur_host.conn, real_key, -1.0, mc->fake), false);
  }
}

bool f$RpcMemcache$$delete(const class_instance<C$RpcMemcache> &mc, const string &key) {
  if (mc->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::delete with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(mc->hosts);
  return catchException(f$rpc_mc_delete(cur_host.conn, real_key, -1.0, mc->fake), false);
}

mixed f$RpcMemcache$$decrement(const class_instance<C$RpcMemcache> &v$this, const string &key, int64_t count) {
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::decrement with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(v$this->hosts);
  mc_method = "decrement";
  return catchException(rpc_mc_run_increment(v$this->fake ? TL_ENGINE_MC_DECR_QUERY : MEMCACHE_DECR, cur_host.conn, real_key, count, -1), false);
}

mixed f$RpcMemcache$$increment(const class_instance<C$RpcMemcache> &v$this, const string &key, int64_t count) {
  if (v$this->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::increment with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  auto cur_host = get_host(v$this->hosts);
  mc_method = "increment";
  return catchException(rpc_mc_run_increment(v$this->fake ? TL_ENGINE_MC_INCR_QUERY : MEMCACHE_INCR, cur_host.conn, real_key, count, -1.0), false);
}

mixed f$RpcMemcache$$getVersion(const class_instance<C$RpcMemcache>& mc) {
  if (mc->hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::getVersion");
    return false;
  }

  php_warning("Method getVersion doesn't supported for object of class RpcMemcache");
  return false;
}


class_instance<C$McMemcache> f$McMemcache$$__construct(const class_instance<C$McMemcache> &v$this) {
  return v$this;
}

class_instance<C$RpcMemcache> f$RpcMemcache$$__construct(const class_instance<C$RpcMemcache> &v$this, bool fake) {
  v$this->fake = fake;
  return v$this;
}

/*
 *
 *  RPC memcached interface
 *
 */

mixed f$rpc_mc_get(const class_instance<C$RpcConnection> &conn, const string &key, double timeout, bool fake) {
  mc_method = "get";
  const string real_key = mc_prepare_key(key);
  bool is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  store_int(fake ? TL_ENGINE_MC_GET_QUERY : MEMCACHE_GET);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

  int64_t request_id = rpc_send(conn, timeout, is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_without_result_synchronously(request_id);
  if (!rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int32_t op = TRY_CALL(int32_t, mixed, rpc_lookup_int());
  if (op == MEMCACHE_ERROR) {
    TRY_CALL_VOID(mixed, rpc_fetch_int());//op
    TRY_CALL_VOID(mixed, f$fetch_long());//query_id
    int32_t error_code = TRY_CALL(int32_t, bool, rpc_fetch_int());
    string error = TRY_CALL(string, bool, f$fetch_string());
    static_cast<void>(error_code);
    return false;
  }
  mixed result = TRY_CALL(mixed, mixed, f$fetch_memcache_value());
  return result;
}

bool rpc_mc_run_set(int32_t op, const class_instance<C$RpcConnection> &conn, const string &key, const mixed &value, int64_t flags, int64_t expire, double timeout) {
  if (flags & ~MEMCACHE_COMPRESSED) {
    php_warning("Wrong parameter flags = %" PRIi64 " in Memcache::%s", flags, mc_method);
    flags &= MEMCACHE_COMPRESSED;
  }

  if ((unsigned int)expire > (unsigned int)(30 * 24 * 60 * 60)) {
    php_warning("Wrong parameter expire = %" PRIi64 " in Memcache::%s", expire, mc_method);
    expire = 0;
  }

  string string_value;
  if (f$is_array(value)) {
    string_value = f$serialize(value);
    flags |= MEMCACHE_SERIALIZED;
  } else {
    string_value = value.to_string();
  }

  if (flags & MEMCACHE_COMPRESSED) {
    string_value = f$gzcompress(string_value);
  }

  if (string_value.size() >= MAX_VALUE_LEN) {
    php_warning("Parameter value has length %d and too large for storing in Memcache", (int)string_value.size());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  bool is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  store_int(op);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);
  f$store_int(flags);
  f$store_int(expire);
  store_string(string_value.c_str(), string_value.size());

  int64_t request_id = rpc_send(conn, timeout, is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_without_result_synchronously(request_id);
  if (!rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int32_t res = TRY_CALL(int32_t, bool, (rpc_fetch_int()));//TODO __FILE__ and __LINE__
  return res == MEMCACHE_TRUE;
}

mixed rpc_mc_run_increment(int op, const class_instance<C$RpcConnection> &conn, const string &key, int64_t v, double timeout) {
  const string real_key = mc_prepare_key(key);
  bool is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  store_int(op);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);
  f$store_long(v);

  int64_t request_id = rpc_send(conn, timeout, is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return 0;
  }

  wait_without_result_synchronously(request_id);
  if (!rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int32_t res = TRY_CALL(int32_t, mixed, (rpc_fetch_int()));//TODO __FILE__ and __LINE__
  if (res == MEMCACHE_VALUE_LONG) {
    return TRY_CALL(mixed, mixed, (f$fetch_long()));
  }

  return false;
}

bool f$rpc_mc_delete(const class_instance<C$RpcConnection> &conn, const string &key, double timeout, bool fake) {
  mc_method = "delete";
  const string real_key = mc_prepare_key(key);
  bool is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  store_int(fake ? TL_ENGINE_MC_DELETE_QUERY : MEMCACHE_DELETE);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

  int64_t request_id = rpc_send(conn, timeout, is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_without_result_synchronously(request_id);
  if (!rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int32_t res = TRY_CALL(int32_t, bool, (rpc_fetch_int()));//TODO __FILE__ and __LINE__
  return res == MEMCACHE_TRUE;
}

static void reset_drivers_global_vars() {
  hard_reset_var(mc_method);
  hard_reset_var(mc_bool_res);
  hard_reset_var(mc_res);
}

void init_memcache_lib() {
  reset_drivers_global_vars();
}

void free_memcache_lib() {
  reset_drivers_global_vars();
}
