#include "runtime/memcache.h"

#include <cstdlib>
#include <ctime>

#include "auto/TL/constants.h"

static_assert(TL_ENGINE_MC_GET_QUERY == ENGINE_MC_GET_QUERY, "bad ENGINE_MC_GET_QUERY constant");

#include "PHP/common-net-functions.h"

#include "runtime/array_functions.h"
#include "runtime/datetime.h"
#include "runtime/interface.h"
#include "runtime/math_functions.h"
#include "runtime/misc.h"
#include "runtime/openssl.h"
#include "runtime/regexp.h"
#include "runtime/string_functions.h"
#include "runtime/zlib.h"

static string_buffer drivers_SB(1024);

const int MAX_KEY_LEN = 1000;
const int MAX_VALUE_LEN = (1 << 20);
const int MAX_INPUT_VALUE_LEN = (1 << 24);

const string UNDERSCORE("_", 1);


const char *mc_method{nullptr};
static const char *mc_last_key{nullptr};
static int mc_last_key_len{0};
static var mc_res;
static bool mc_bool_res{false};


const string mc_prepare_key(const string &key) {
  if (key.size() < 3) {
    php_warning("Very short key \"%s\" in Memcache::%s", key.c_str(), mc_method);
  }

  bool bad_key = ((int)key.size() > MAX_KEY_LEN || key.empty());
  for (int i = 0; i < (int)key.size() && !bad_key; i++) {
    if ((unsigned int)key[i] <= 32u) {
      bad_key = true;
    }
  }
  if (!bad_key) {
    return key;
  }

  string real_key = key.substr(0, min((dl::size_type)MAX_KEY_LEN, key.size()));//need a copy
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
  return key[0] == '^' && (int)key.size() >= 2;
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
  if (!php_try_to_int(*value, (int)(result + i - *value), flags)) {
    *error_code = 6;
    return nullptr;
  }

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
  if (!php_try_to_int(*value, (int)(result + i - *value), value_len) || (unsigned int)(*value_len) >= (unsigned int)MAX_INPUT_VALUE_LEN) {
    *error_code = 9;
    return nullptr;
  }

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

var mc_get_value(const char *result_str, int result_str_len, int flags) {
  var result;
  if (flags & MEMCACHE_COMPRESSED) {
    flags ^= MEMCACHE_COMPRESSED;
    string::size_type uncompressed_len;
    result_str = gzuncompress_raw(result_str, result_str_len, &uncompressed_len);
    result_str_len = uncompressed_len;
  }

  if (flags & MEMCACHE_SERIALIZED) {
    flags ^= MEMCACHE_SERIALIZED;
    result = unserialize_raw(result_str, result_str_len);
  } else {
    result = string(result_str, result_str_len);
  }

  if (flags) {
//    php_warning ("Wrong parameter flags %d with value \"%s\" returned in Memcache::get", flags, result_str.c_str());
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


McMemcache::host::host() :
  host_num(-1),
  host_port(-1),
  host_weight(0),
  timeout_ms(200) {
}

McMemcache::host::host(int host_num, int host_port, int host_weight, int timeout_ms) :
  host_num(host_num),
  host_port(host_port),
  host_weight(host_weight),
  timeout_ms(timeout_ms) {
}


McMemcache::host McMemcache::get_host(const string &key __attribute__((unused))) {
  php_assert (hosts.count() > 0);

  return hosts.get_value(f$array_rand(hosts));
}


bool McMemcache::run_set(const string &key, const var &value, int flags, int expire) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::%s with key \"%s\"", mc_method, key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);

  if (flags & ~MEMCACHE_COMPRESSED) {
    php_warning("Wrong parameter flags = %d in Memcache::%s", flags, mc_method);
    flags &= MEMCACHE_COMPRESSED;
  }

  if ((unsigned int)expire > (unsigned int)(30 * 24 * 60 * 60)) {
    php_warning("Wrong parameter expire = %d in Memcache::%s", expire, mc_method);
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

  if (string_value.size() >= (dl::size_type)MAX_VALUE_LEN) {
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
  host cur_host = get_host(real_key);
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

var McMemcache::run_increment(const string &key, const var &count) {
  if (hosts.count() <= 0) {
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
  host cur_host = get_host(real_key);
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

McMemcache::McMemcache() :
  hosts(array_size(1, 0, true)) {
}

bool McMemcache::addServer(const string &host_name, int port, bool persistent __attribute__((unused)), int weight, int timeout, int retry_interval __attribute__((unused)), bool status __attribute__((unused)), const var &failure_callback __attribute__((unused)), int timeoutms) {
  if (timeout <= 0) {
    timeout = 1;
  }
  if (timeout >= MAX_TIMEOUT) {
    timeout = MAX_TIMEOUT;
  }

  php_assert (MAX_TIMEOUT < 1000000);
  if (1 <= timeoutms && timeoutms <= 1000 * MAX_TIMEOUT) {
    timeout = timeoutms;
  } else {
    timeout *= 1000;
  }

  int host_num = mc_connect_to(host_name.c_str(), port);
  if (host_num >= 0) {
    hosts.push_back(host(host_num, port, weight, timeout));
    return true;
  }
  return false;
}

bool McMemcache::connect(const string &host_name, int port, int timeout) {
  return addServer(host_name, port, false, 1, timeout, 15, true, var(), -1);
}

bool McMemcache::pconnect(const string &host_name, int port, int timeout) {
  return addServer(host_name, port, true, 1, timeout, 15, true, var(), -1);
}

bool McMemcache::rpc_connect(const string &host_name __attribute__((unused)), int port __attribute__((unused)), const var &default_actor_id __attribute__((unused)), double timeout __attribute__((unused)), double connect_timeout __attribute__((unused)), double reconnect_timeout __attribute__((unused))) {
  php_warning("Method rpc_connect doesn't supported for object of class Memcache");
  return false;
}


bool McMemcache::add(const string &key, const var &value, int flags, int expire) {
  mc_method = "add";
  return run_set(key, value, flags, expire);
}

bool McMemcache::set(const string &key, const var &value, int flags, int expire) {
  mc_method = "set";
  return run_set(key, value, flags, expire);
}

bool McMemcache::replace(const string &key, const var &value, int flags, int expire) {
  mc_method = "replace";
  return run_set(key, value, flags, expire);
}

var McMemcache::get(const var &key_var) {
  mc_method = "get";
  if (f$is_array(key_var)) {
    if (hosts.count() <= 0) {
      php_warning("There is no available server to run Memcache::get");
      return array<var>();
    }

    drivers_SB.clean();
    drivers_SB << "get";
    bool is_immediate_query = true;
    for (array<var>::const_iterator p = key_var.begin(); p != key_var.end(); ++p) {
      const string key = p.get_value().to_string();
      const string real_key = mc_prepare_key(key);
      drivers_SB << ' ' << real_key;
      is_immediate_query = is_immediate_query && mc_is_immediate_query(real_key);
    }
    drivers_SB << "\r\n";

    mc_res = array<var>(array_size(0, key_var.count(), false));
    host cur_host = get_host(string());
    if (is_immediate_query) {
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, nullptr); //TODO wrong if we have no mc_proxy
    } else {
      mc_last_key = drivers_SB.c_str();
      mc_last_key_len = (int)drivers_SB.size();
      mc_run_query(cur_host.host_num, drivers_SB.c_str(), drivers_SB.size(), cur_host.timeout_ms, 0, mc_multiget_callback); //TODO wrong if we have no mc_proxy
    }
  } else {
    if (hosts.count() <= 0) {
      php_warning("There is no available server to run Memcache::get with key \"%s\"", key_var.to_string().c_str());
      return false;
    }

    const string key = key_var.to_string();
    const string real_key = mc_prepare_key(key);

    drivers_SB.clean() << "get " << real_key << "\r\n";

    host cur_host = get_host(real_key);
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

bool McMemcache::delete_(const string &key) {
  mc_method = "delete";
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::delete with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);

  drivers_SB.clean() << "delete " << real_key << "\r\n";

  mc_bool_res = false;
  host cur_host = get_host(real_key);
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

var McMemcache::decrement(const string &key, const var &count) {
  mc_method = "decr";
  return run_increment(key, count);
}

var McMemcache::increment(const string &key, const var &count) {
  mc_method = "incr";
  return run_increment(key, count);
}

var McMemcache::getVersion() {
  static const char *version_str = "version\r\n";
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run Memcache::getVersion");
    return false;
  }

  mc_res = false;
  host cur_host = get_host(string());
  mc_run_query(cur_host.host_num, version_str, (int)strlen(version_str), cur_host.timeout_ms, 1, mc_version_callback);

  return mc_res;
}


RpcMemcache::host::host(const string &host_name, int port, int actor_id, int host_weight, int timeout_ms) :
  conn(f$new_rpc_connection(host_name, port, actor_id, timeout_ms * 0.001)),
  host_weight(host_weight),
  actor_id(actor_id) {
}

RpcMemcache::host::host(const rpc_connection &c) :
  conn(c),
  host_weight(1),
  actor_id(-1) {
}

RpcMemcache::host RpcMemcache::get_host(const string &key __attribute__((unused))) {
  php_assert (hosts.count() > 0);

  return hosts.get_value(f$array_rand(hosts));
}

RpcMemcache::RpcMemcache(bool fake) :
  hosts(array_size(1, 0, true)),
  fake(fake) {
}

bool RpcMemcache::addServer(const string &host_name, int port, bool persistent __attribute__((unused)), int weight, int timeout, int retry_interval, bool status __attribute__((unused)), const var &failure_callback __attribute__((unused)), int timeoutms) {
  if (timeout <= 0) {
    timeout = 1;
  }
  if (timeout >= MAX_TIMEOUT) {
    timeout = MAX_TIMEOUT;
  }

  php_assert (MAX_TIMEOUT < 1000000);
  if (1 <= timeoutms && timeoutms <= 1000 * MAX_TIMEOUT) {
    timeout = timeoutms;
  } else {
    timeout *= 1000;
  }

  host new_host = host(host_name, port, retry_interval >= 100 ? retry_interval : 0, weight, timeout);
  if (new_host.conn.host_num >= 0) {
    hosts.push_back(new_host);
    return true;
  }
  return false;
}

bool RpcMemcache::connect(const string &host_name, int port, int timeout) {
  return addServer(host_name, port, false, 1, timeout, 15, true, var(), -1);
}

bool RpcMemcache::pconnect(const string &host_name, int port, int timeout) {
  return addServer(host_name, port, true, 1, timeout, 15, true, var(), -1);
}

bool RpcMemcache::rpc_connect(const string &host_name, int port, const var &default_actor_id, double timeout, double connect_timeout, double reconnect_timeout) {
  rpc_connection c = f$new_rpc_connection(host_name, port, default_actor_id, timeout, connect_timeout, reconnect_timeout);
  if (c.host_num >= 0) {
    host h = host(c);
    h.actor_id = default_actor_id.to_int();
    hosts.push_back(h);
    return true;
  }
  return false;
}


bool RpcMemcache::add(const string &key, const var &value, int flags, int expire) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::add with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  bool res = f$rpc_mc_add(cur_host.conn, real_key, value, flags, expire, -1.0, fake);
  return res;
}

bool RpcMemcache::set(const string &key, const var &value, int flags, int expire) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::set with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  bool res = f$rpc_mc_set(cur_host.conn, real_key, value, flags, expire, -1.0, fake);
  return res;
}

bool RpcMemcache::replace(const string &key, const var &value, int flags, int expire) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::replace with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  bool res = f$rpc_mc_replace(cur_host.conn, real_key, value, flags, expire, -1.0, fake);
  return res;
}

var RpcMemcache::get(const var &key_var) {
  if (f$is_array(key_var)) {
    if (hosts.count() <= 0) {
      php_warning("There is no available server to run RpcMemcache::get");
      return array<var>();
    }

    host cur_host = get_host(string());
    var res = f$rpc_mc_multiget(cur_host.conn, key_var.to_array(), -1.0, false, true, fake);
    php_assert(resumable_finished);
    return res;
  } else {
    if (hosts.count() <= 0) {
      php_warning("There is no available server to run RpcMemcache::get with key \"%s\"", key_var.to_string().c_str());
      return false;
    }

    const string key = key_var.to_string();
    const string real_key = mc_prepare_key(key);

    host cur_host = get_host(real_key);
    var res = f$rpc_mc_get(cur_host.conn, real_key, -1.0, fake);
    return res;
  }
}

bool RpcMemcache::delete_(const string &key) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::delete with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  bool res = f$rpc_mc_delete(cur_host.conn, real_key, -1.0, fake);
  return res;
}

var RpcMemcache::decrement(const string &key, const var &count) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::decrement with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  var res = f$rpc_mc_decrement(cur_host.conn, real_key, count, -1.0, fake);
  return res;
}

var RpcMemcache::increment(const string &key, const var &count) {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::increment with key \"%s\"", key.c_str());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  host cur_host = get_host(real_key);
  var res = f$rpc_mc_increment(cur_host.conn, real_key, count, -1.0, fake);
  return res;
}

var RpcMemcache::getVersion() {
  if (hosts.count() <= 0) {
    php_warning("There is no available server to run RpcMemcache::getVersion");
    return false;
  }

  php_warning("Method getVersion doesn't supported for object of class RpcMemcache");
  return false;
}


bool f$Memcache$$addServer(const Memcache &mc, const string &host_name, int port, bool persistent, int weight, double timeout, int retry_interval, bool status, const var &failure_callback, int timeoutms) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->addServer");
    return false;
  }

  int timeout_s = static_cast<int>(timeout);
  int timeout_ms_from_timeout = static_cast<int>((timeout - timeout_s) * 1000);
  if (timeout_ms_from_timeout > 0 && timeoutms > 0) {
    php_warning("Memcache::addServer doesn't support timeoutms parameter with float timeout = %lf", timeout);
    return false;
  } else if (timeout_ms_from_timeout == 0) {
    timeout_ms_from_timeout = timeoutms;
  } else {
    timeout_ms_from_timeout += timeout_s * 1000;
  }

  return mc.mc->addServer(host_name, port, persistent, weight, timeout_s, retry_interval, status, failure_callback, timeout_ms_from_timeout);
}

bool f$Memcache$$connect(const Memcache &mc, const string &host_name, int port, int timeout) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->connect");
    return false;
  }
  return mc.mc->connect(host_name, port, timeout);
}

bool f$Memcache$$pconnect(const Memcache &mc, const string &host_name, int port, int timeout) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->pconnect");
    return false;
  }
  return mc.mc->pconnect(host_name, port, timeout);
}

bool f$Memcache$$rpc_connect(const Memcache &mc, const string &host_name, int port, const var &default_actor_id, double timeout, double connect_timeout, double reconnect_timeout) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->rpc_connect");
    return false;
  }
  return mc.mc->rpc_connect(host_name, port, default_actor_id, timeout, connect_timeout, reconnect_timeout);
}


bool f$Memcache$$add(const Memcache &mc, const string &key, const var &value, int flags, int expire) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->add");
    return false;
  }
  return mc.mc->add(key, value, flags, expire);
}

bool f$Memcache$$set(const Memcache &mc, const string &key, const var &value, int flags, int expire) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->set");
    return false;
  }
  return mc.mc->set(key, value, flags, expire);
}

bool f$Memcache$$replace(const Memcache &mc, const string &key, const var &value, int flags, int expire) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->replace");
    return false;
  }
  return mc.mc->replace(key, value, flags, expire);
}

var f$Memcache$$get(const Memcache &mc, const var &key_var) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->get");
    return key_var.is_array() ? var(array<var>()) : var(false);
  }
  return mc.mc->get(key_var);
}

bool f$Memcache$$delete(const Memcache &mc, const string &key) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->delete");
    return false;
  }
  return mc.mc->delete_(key);
}

var f$Memcache$$decrement(const Memcache &mc, const string &key, const var &v) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->decrement");
    return false;
  }
  return mc.mc->decrement(key, v);
}

var f$Memcache$$increment(const Memcache &mc, const string &key, const var &v) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->increment");
    return false;
  }
  return mc.mc->increment(key, v);
}

var f$Memcache$$getVersion(const Memcache &mc) {
  if (mc.mc == nullptr) {
    php_warning("Memcache object is NULL in Memcache->getVersion");
    return false;
  }
  return mc.mc->getVersion();
}


bool f$boolval(const Memcache &my_mc) {
  return f$boolval(my_mc.bool_value);
}

bool eq2(const Memcache &my_mc, bool value) {
  return my_mc.bool_value == value;
}

bool eq2(bool value, const Memcache &my_mc) {
  return value == my_mc.bool_value;
}

bool eq2(const Memcache &mc1, const Memcache &mc2) {
  return mc1.bool_value == mc2.bool_value && mc1.mc == mc2.mc;
}

bool equals(bool value, const Memcache &my_mc) {
  return equals(value, my_mc.bool_value);
}

bool equals(const Memcache &my_mc, bool value) {
  return equals(my_mc.bool_value, value);
}

bool equals(const Memcache &mc1, const Memcache &mc2) {
  return mc1.bool_value == mc2.bool_value && mc1.mc == mc2.mc;
}


Memcache &Memcache::operator=(bool value) {
  bool_value = value;
  mc = nullptr;
  return *this;
}

Memcache::Memcache(bool value) :
  bool_value(value),
  mc(nullptr) {}

Memcache::Memcache(MC_object *mc) :
  bool_value(true),
  mc(mc) {
}

Memcache::Memcache() :
  bool_value(),
  mc(nullptr) {
}

Memcache f$Memcache$$__construct() {
  void *buf = dl::allocate(sizeof(McMemcache));
  return Memcache(new(buf) McMemcache());
}

Memcache f$new_RpcMemcache(bool fake) {
  void *buf = dl::allocate(sizeof(RpcMemcache));
  return Memcache(new(buf) RpcMemcache(fake));
}

/*
 *
 *  RPC memcached interface
 *
 */

var f$rpc_mc_get(const rpc_connection &conn, const string &key, double timeout, bool fake) {
  mc_method = "get";
  const string real_key = mc_prepare_key(key);
  int is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  f$store_int(fake ? TL_ENGINE_MC_GET_QUERY : MEMCACHE_GET);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

  int request_id = rpc_send(conn, timeout, (bool)is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_synchronously(request_id);
  if (!f$rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int op = TRY_CALL(int, var, rpc_lookup_int());
  if (op == MEMCACHE_ERROR) {
    TRY_CALL_VOID(var, f$fetch_int());//op
    TRY_CALL_VOID(var, f$fetch_long());//query_id
    int error_code = TRY_CALL(int, bool, f$fetch_int());
    string error = TRY_CALL(string, bool, f$fetch_string());
    static_cast<void>(error_code);
    return false;
  }
  var result = TRY_CALL(var, var, f$fetch_memcache_value());
  return result;
}

bool rpc_mc_run_set(int op, const rpc_connection &conn, const string &key, const var &value, int flags, int expire, double timeout) {
  if (flags & ~MEMCACHE_COMPRESSED) {
    php_warning("Wrong parameter flags = %d in Memcache::%s", flags, mc_method);
    flags &= MEMCACHE_COMPRESSED;
  }

  if ((unsigned int)expire > (unsigned int)(30 * 24 * 60 * 60)) {
    php_warning("Wrong parameter expire = %d in Memcache::%s", expire, mc_method);
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

  if (string_value.size() >= (dl::size_type)MAX_VALUE_LEN) {
    php_warning("Parameter value has length %d and too large for storing in Memcache", (int)string_value.size());
    return false;
  }

  const string real_key = mc_prepare_key(key);
  int is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  f$store_int(op);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);
  f$store_int(flags);
  f$store_int(expire);
  store_string(string_value.c_str(), string_value.size());

  int request_id = rpc_send(conn, timeout, (bool)is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_synchronously(request_id);
  if (!f$rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int res = TRY_CALL(int, bool, (f$fetch_int()));//TODO __FILE__ and __LINE__
  return res == MEMCACHE_TRUE;
}

bool f$rpc_mc_set(const rpc_connection &conn, const string &key, const var &value, int flags, int expire, double timeout, bool fake) {
  mc_method = "set";
  return rpc_mc_run_set(fake ? TL_ENGINE_MC_SET_QUERY : MEMCACHE_SET, conn, key, value, flags, expire, timeout);
}

bool f$rpc_mc_add(const rpc_connection &conn, const string &key, const var &value, int flags, int expire, double timeout, bool fake) {
  mc_method = "add";
  return rpc_mc_run_set(fake ? TL_ENGINE_MC_ADD_QUERY : MEMCACHE_ADD, conn, key, value, flags, expire, timeout);
}

bool f$rpc_mc_replace(const rpc_connection &conn, const string &key, const var &value, int flags, int expire, double timeout, bool fake) {
  mc_method = "replace";
  return rpc_mc_run_set(fake ? TL_ENGINE_MC_REPLACE_QUERY : MEMCACHE_REPLACE, conn, key, value, flags, expire, timeout);
}

var rpc_mc_run_increment(int op, const rpc_connection &conn, const string &key, const var &v, double timeout) {
  const string real_key = mc_prepare_key(key);
  int is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  f$store_int(op);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);
  f$store_long(v);

  int request_id = rpc_send(conn, timeout, (bool)is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return 0;
  }

  wait_synchronously(request_id);
  if (!f$rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int res = TRY_CALL(int, var, (f$fetch_int()));//TODO __FILE__ and __LINE__
  if (res == MEMCACHE_VALUE_LONG) {
    return TRY_CALL(var, var, (f$fetch_long()));
  }

  return false;
}

var f$rpc_mc_increment(const rpc_connection &conn, const string &key, const var &v, double timeout, bool fake) {
  mc_method = "increment";
  return rpc_mc_run_increment(fake ? TL_ENGINE_MC_INCR_QUERY : MEMCACHE_INCR, conn, key, v, timeout);
}

var f$rpc_mc_decrement(const rpc_connection &conn, const string &key, const var &v, double timeout, bool fake) {
  mc_method = "decrement";
  return rpc_mc_run_increment(fake ? TL_ENGINE_MC_DECR_QUERY : MEMCACHE_DECR, conn, key, v, timeout);
}

bool f$rpc_mc_delete(const rpc_connection &conn, const string &key, double timeout, bool fake) {
  mc_method = "delete";
  const string real_key = mc_prepare_key(key);
  int is_immediate = mc_is_immediate_query(real_key);

  f$rpc_clean();
  f$store_int(fake ? TL_ENGINE_MC_DELETE_QUERY : MEMCACHE_DELETE);
  store_string(real_key.c_str() + is_immediate, real_key.size() - is_immediate);

  int request_id = rpc_send(conn, timeout, (bool)is_immediate);
  if (request_id <= 0) {
    return false;
  }
  f$rpc_flush();
  if (is_immediate) {
    return true;
  }

  wait_synchronously(request_id);
  if (!f$rpc_get_and_parse(request_id, timeout)) {
    php_assert (resumable_finished);
    return false;
  }

  int res = TRY_CALL(int, bool, (f$fetch_int()));//TODO __FILE__ and __LINE__
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
