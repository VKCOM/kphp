#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "runtime/rpc.h"

void drivers_init_static (void);
void drivers_free_static (void);

extern var (*base128DecodeMixed_pointer) (var str);
extern var (*debugServerLog_pointer) (array <var>);
extern var (*debugLogPlain_pointer) (string section, string text);

extern string drivers_h_filename;

const string mc_prepare_key (const string &key);

var mc_get_value (const char *result_str, int result_str_len, int flags);

bool mc_is_immediate_query (const string &key);


const int MEMCACHE_SERIALIZED = 1;
const int MEMCACHE_COMPRESSED = 2;

class MC_object {
protected:
  virtual ~MC_object();

public:
  virtual bool addServer (const string &host_name, int port = 11211, bool persistent = true, int weight = 1, int timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = -1) = 0;
  virtual bool connect (const string &host_name, int port = 11211, int timeout = 1) = 0;
  virtual bool pconnect (const string &host_name, int port = 11211, int timeout = 1) = 0;
  virtual bool rpc_connect (const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17) = 0;

  virtual bool add (const string &key, const var &value, int flags = 0, int expire = 0) = 0;
  virtual bool set (const string &key, const var &value, int flags = 0, int expire = 0) = 0;
  virtual bool replace (const string &key, const var &value, int flags = 0, int expire = 0) = 0;

  virtual var get (const var &key_var) = 0;

  virtual bool delete_ (const string &key) = 0;

  virtual var decrement (const string &key, const var &v = 1) = 0;
  virtual var increment (const string &key, const var &v = 1) = 0;

  virtual var getVersion (void) = 0;

  virtual var getTag (void) const = 0;
  virtual var getLastQueryTime (void) const = 0;

  virtual void bufferNextLog (void) = 0;
  virtual void clearLogBuffer (void) = 0;
  virtual void flushLogBuffer (void) = 0;
};

class Memcache: public MC_object {
private:
  class host {
  public:
    int host_num;
    int host_port;
    int host_weight;
    int timeout_ms;

    host (void);
    host (int host_num, int host_port, int host_weight, int timeout_ms);
  };

  array <host> hosts;


  inline host get_host (const string &key);

  bool run_set (const string &key, const var &value, int flags, int expire);

  var run_increment (const string &key, const var &count);

public:
  Memcache (void);

  bool addServer (const string &host_name, int port = 11211, bool persistent = true, int weight = 1, int timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = -1);
  bool connect (const string &host_name, int port = 11211, int timeout = 1);
  bool pconnect (const string &host_name, int port = 11211, int timeout = 1);
  bool rpc_connect (const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

  bool add (const string &key, const var &value, int flags = 0, int expire = 0);
  bool set (const string &key, const var &value, int flags = 0, int expire = 0);
  bool replace (const string &key, const var &value, int flags = 0, int expire = 0);

  var get (const var &key_var);

  bool delete_ (const string &key);

  var decrement (const string &key, const var &v = 1);
  var increment (const string &key, const var &v = 1);

  var getVersion (void);

  var getTag (void) const;
  var getLastQueryTime (void) const;

  void bufferNextLog (void);
  void clearLogBuffer (void);
  void flushLogBuffer (void);
};

class RpcMemcache: public MC_object {
private:
  class host {
  public:
    rpc_connection conn;
    int host_weight;
    int actor_id;

    host (void);
    host (const string &host_name, int port, int actor_id, int host_weight, int timeout_ms);
    host (const rpc_connection &c);
  };

  array <host> hosts;

  inline host get_host (const string &key);

public:
  RpcMemcache (void);

  bool addServer (const string &host_name, int port = 11211, bool persistent = true, int weight = 1, int timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = -1);
  bool connect (const string &host_name, int port = 11211, int timeout = 1);
  bool pconnect (const string &host_name, int port = 11211, int timeout = 1);
  bool rpc_connect (const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

  bool add (const string &key, const var &value, int flags = 0, int expire = 0);
  bool set (const string &key, const var &value, int flags = 0, int expire = 0);
  bool replace (const string &key, const var &value, int flags = 0, int expire = 0);

  var get (const var &key_var);

  bool delete_ (const string &key);

  var decrement (const string &key, const var &v = 1);
  var increment (const string &key, const var &v = 1);

  var getVersion (void);

  var getTag (void) const;
  var getLastQueryTime (void) const;

  void bufferNextLog (void);
  void clearLogBuffer (void);
  void flushLogBuffer (void);
};

class true_mc: public MC_object {
private:
  int server_rand;

  MC_object *mc;
  string engine_tag;
  string engine_name;
  bool is_debug;
  bool is_debug_empty;
  double query_time_threshold;

  int query_index;
  double last_query_time;

  bool is_true_mc;

  int use_log_buffer;
  array <string> log_buffer_key;
  array <array <var> > log_buffer_value;
  array <int> log_buffer_expire;

  void check_result (const char *operation, const var &key_var, double time_diff, const var &result);

public:
  true_mc (MC_object *mc, bool is_true_mc, const string &engine_tag = string(), const string &engine_name = string(), bool is_debug = false, bool is_debug_empty = false, double query_time_threshold = 0.0);

  bool addServer (const string &host_name, int port = 11211, bool persistent = true, int weight = 1, int timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = -1);
  bool connect (const string &host_name, int port = 11211, int timeout = 1);
  bool pconnect (const string &host_name, int port = 11211, int timeout = 1);
  bool rpc_connect (const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

  bool add (const string &key, const var &value, int flags = 0, int expire = 0);
  bool set (const string &key, const var &value, int flags = 0, int expire = 0);
  bool replace (const string &key, const var &value, int flags = 0, int expire = 0);

  var get (const var &key_var);

  bool delete_ (const string &key);

  var decrement (const string &key, const var &v = 1);
  var increment (const string &key, const var &v = 1);

  var getVersion (void);

  var getTag (void) const;
  var getLastQueryTime (void) const;

  void bufferNextLog (void);
  void clearLogBuffer (void);
  void flushLogBuffer (void);
};

class MyMemcache {
private:
  bool bool_value;
  MC_object *mc;

  MyMemcache (MC_object *mc);
public:
  MyMemcache (void);

  friend bool f$memcached_addServer (const MyMemcache &mc, const string &host_name, int port, bool persistent, int weight, int timeout, int retry_interval, bool status, const var &failure_callback, int timeoutms);
  friend bool f$memcached_connect (const MyMemcache &mc, const string &host_name, int port, int timeout);
  friend bool f$memcached_pconnect (const MyMemcache &mc, const string &host_name, int port, int timeout);
  friend bool f$memcached_rpc_connect (const MyMemcache &mc, const string &host_name, int port, const var &default_actor_id, double timeout, double connect_timeout, double reconnect_timeout);

  friend bool f$memcached_add (const MyMemcache &mc, const string &key, const var &value, int flags, int expire);
  friend bool f$memcached_set (const MyMemcache &mc, const string &key, const var &value, int flags, int expire);
  friend bool f$memcached_replace (const MyMemcache &mc, const string &key, const var &value, int flags, int expire);

  friend var f$memcached_get (const MyMemcache &mc, const var &key_var);

  friend bool f$memcached_delete (const MyMemcache &mc, const string &key);

  friend var f$memcached_decrement (const MyMemcache &mc, const string &key, const var &v);
  friend var f$memcached_increment (const MyMemcache &mc, const string &key, const var &v);

  friend var f$memcached_getVersion (const MyMemcache &mc);

  friend var f$memcached_getTag (const MyMemcache &mc);
  friend var f$memcached_getLastQueryTime (const MyMemcache &mc);

  friend void f$memcached_bufferNextLog (const MyMemcache &mc);
  friend void f$memcached_clearLogBuffer (const MyMemcache &mc);
  friend void f$memcached_flushLogBuffer (const MyMemcache &mc);

  friend bool f$boolval (const MyMemcache &my_mc);
  friend bool eq2 (const MyMemcache &my_mc, bool value);
  friend bool eq2 (bool value, const MyMemcache &my_mc);
  friend bool equals (bool value, const MyMemcache &my_mc);
  friend bool equals (const MyMemcache &my_mc, bool value);
  friend bool not_equals (bool value, const MyMemcache &my_mc);
  friend bool not_equals (const MyMemcache &my_mc, bool value);

  MyMemcache& operator = (bool value);
  MyMemcache (bool value);

  friend array <string> f$mcGetStats (const MyMemcache &MC);

  friend int f$mcGetClusterSize (const MyMemcache &MC);


  friend MyMemcache f$new_Memcache (void);
  friend MyMemcache f$new_RpcMemcache (void);
  friend MyMemcache f$new_true_mc (const MyMemcache &mc, const string &engine_tag, const string &engine_name, bool is_debug, bool is_debug_empty, double query_time_threshold);
  friend MyMemcache f$new_test_mc (const MyMemcache &mc, const string &engine_tag);
  friend MyMemcache f$new_rich_mc (const MyMemcache &mc, const string &engine_tag);
};

bool f$memcached_addServer (const MyMemcache &mc, const string &host_name, int port = 11211, bool persistent = true, int weight = 1, int timeout = 1, int retry_interval = 15, bool status = true, const var &failure_callback = var(), int timeoutms = -1);
bool f$memcached_connect (const MyMemcache &mc, const string &host_name, int port = 11211, int timeout = 1);
bool f$memcached_pconnect (const MyMemcache &mc, const string &host_name, int port = 11211, int timeout = 1);
bool f$memcached_rpc_connect (const MyMemcache &mc, const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

bool f$memcached_add (const MyMemcache &mc, const string &key, const var &value, int flags = 0, int expire = 0);
bool f$memcached_set (const MyMemcache &mc, const string &key, const var &value, int flags = 0, int expire = 0);
bool f$memcached_replace (const MyMemcache &mc, const string &key, const var &value, int flags = 0, int expire = 0);

var f$memcached_get (const MyMemcache &mc, const var &key_var);

bool f$memcached_delete (const MyMemcache &mc, const string &key);

var f$memcached_decrement (const MyMemcache &mc, const string &key, const var &v = 1);
var f$memcached_increment (const MyMemcache &mc, const string &key, const var &v = 1);

var f$memcached_getVersion (const MyMemcache &mc);

var f$memcached_getTag (const MyMemcache &mc);
var f$memcached_getLastQueryTime (const MyMemcache &mc);

void f$memcached_bufferNextLog (const MyMemcache &mc);
void f$memcached_clearLogBuffer (const MyMemcache &mc);
void f$memcached_flushLogBuffer (const MyMemcache &mc);

bool f$boolval (const MyMemcache &my_mc);
bool eq2 (const MyMemcache &my_mc, bool value);
bool eq2 (bool value, const MyMemcache &my_mc);
bool equals (bool value, const MyMemcache &my_mc);
bool equals (const MyMemcache &my_mc, bool value);
bool not_equals (bool value, const MyMemcache &my_mc);
bool not_equals (const MyMemcache &my_mc, bool value);

array <string> f$mcGetStats (const MyMemcache &MC);

int f$mcGetClusterSize (const MyMemcache &MC);

MyMemcache f$new_Memcache (void);
MyMemcache f$new_RpcMemcache (void);
MyMemcache f$new_true_mc (const MyMemcache &mc, const string &engine_tag = string(), const string &engine_name = string(), bool is_debug = false, bool is_debug_empty = false, double query_time_threshold = 0.0);
MyMemcache f$new_test_mc (const MyMemcache &mc, const string &engine_tag = string());
MyMemcache f$new_rich_mc (const MyMemcache &mc, const string &engine_tag = string());


var f$rpc_mc_get (const rpc_connection &conn, const string &key, double timeout = -1.0);

template <class T>
OrFalse <array <var> > f$rpc_mc_multiget (const rpc_connection &conn, const array <T> &keys, double timeout = -1.0, bool return_false_if_not_found = false, bool run_synchronously = false);

bool f$rpc_mc_set (const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0);

bool f$rpc_mc_add (const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0);

bool f$rpc_mc_replace (const rpc_connection &conn, const string &key, const var &value, int flags = 0, int expire = 0, double timeout = -1.0);

var f$rpc_mc_increment (const rpc_connection &conn, const string &key, const var &v = 1, double timeout = -1.0);

var f$rpc_mc_decrement (const rpc_connection &conn, const string &key, const var &v = 1, double timeout = -1.0);

bool f$rpc_mc_delete (const rpc_connection &conn, const string &key, double timeout = -1.0);


class MyDB;

class db_driver {
private:
  int connection_id;
  int connected; // K.O.T.: 1 = connected, -1 = error, -2 = down

  int last_query_id;
  int biggest_query_id;
  string error;
  int errno_;
  int affected_rows;
  int insert_id;
  array <array <array <var> > > query_results;
  array <int> cur_pos;

  int field_cnt;
  array <string> field_names;

public:
  explicit db_driver();

  void do_connect_no_log (void);

  int get_affected_rows (void);

  int get_num_rows (void);

  int get_insert_id (void);

  bool mysql_query (const string &query);

  var mysql_query_update_last (const string &query_string);

  OrFalse <array <var> > mysql_fetch_array (int query_id);


  friend string f$mysqli_error(const MyDB &db);

  friend int f$mysqli_errno(const MyDB &db);

  friend int db_get_num_rows (const MyDB &db, int id);

  friend MyDB f$vk_mysqli_connect(const string &host __attribute__((unused)), int port __attribute__((unused)));
};

class MyDB {
private:
  bool bool_value;
  db_driver *db;

  explicit MyDB (db_driver *db);
public:
  MyDB (void);

  friend void db_do_connect_no_log (const MyDB &db);

  friend var db_mysql_query (const MyDB &db, const string &query);

  friend int db_get_affected_rows (const MyDB &db);

  friend int db_get_insert_id (const MyDB &db);

  friend OrFalse <array <var> > db_fetch_array(const MyDB &db, int query_id);

  friend bool f$boolval (const MyDB &my_db);
  friend bool eq2 (const MyDB &my_db, bool value);
  friend bool eq2 (bool value, const MyDB &my_db);
  friend bool equals (bool value, const MyDB &my_db);
  friend bool equals (const MyDB &my_db, bool value);
  friend bool not_equals (bool value, const MyDB &my_db);
  friend bool not_equals (const MyDB &my_db, bool value);

  MyDB& operator = (bool value);
  explicit MyDB (bool value);

  friend MyDB f$vk_mysqli_connect(const string &host, int port);

  friend int db_get_num_rows (const MyDB &db, int id);

  friend string f$mysqli_error(const MyDB &db);

  friend int f$mysqli_errno(const MyDB &db);
};

int db_get_affected_rows (const MyDB &db);

int db_get_num_rows (const MyDB &db, int id = -1);

int db_get_insert_id (const MyDB &db);

bool f$boolval (const MyDB &my_db);
bool eq2 (const MyDB &my_db, bool value);
bool eq2 (bool value, const MyDB &my_db);
bool equals (bool value, const MyDB &my_db);
bool equals (const MyDB &my_db, bool value);
bool not_equals (bool value, const MyDB &my_db);
bool not_equals (const MyDB &my_db, bool value);


string f$mysqli_error(const MyDB &db);

int f$mysqli_errno(const MyDB &db);


int f$mysqli_affected_rows(const MyDB &dn);

OrFalse< array <var> > f$mysqli_fetch_array(int query_id_var, int result_type);

int f$mysqli_insert_id(const MyDB &dn);

int f$mysqli_num_rows(int query_id);

var f$mysqli_query(const MyDB &dn, const string &query);

MyDB f$vk_mysqli_connect(const string &host, int port);

bool f$mysqli_select_db(const MyDB &dn, const string& name);


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

extern const char *mc_method;

class rpc_mc_multiget_resumable: public Resumable {
  typedef OrFalse <array <var> > ReturnT;

  int queue_id;
  int first_request_id;
  int keys_n;
  int request_id;
  array <string> query_names;
  array <var> result;
  bool return_false_if_not_found;

protected:
  bool run (void) {
    RESUMABLE_BEGIN
      while (keys_n > 0) {
        request_id = f$wait_queue_next (queue_id, -1);
        TRY_WAIT(rpc_mc_multiget_resumable_label_0, request_id, int);

        if (request_id <= 0) {
          break;
        }
        keys_n--;

        int k = (int)(request_id - first_request_id);
        php_assert ((unsigned int)k < (unsigned int)query_names.count());

        bool parse_result = f$rpc_get_and_parse (request_id, -1);
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
          php_warning ("Not all data fetched during fetch memcache.Value");
        }
      }
      RETURN(result);
    RESUMABLE_END
  }

public:
  rpc_mc_multiget_resumable (int queue_id, int first_request_id, int keys_n, array <string> query_names, bool return_false_if_not_found):
      queue_id (queue_id),
      first_request_id (first_request_id),
      keys_n (keys_n),
      request_id (-1),
      query_names (query_names),
      result (array_size (0, keys_n, false)),
      return_false_if_not_found (return_false_if_not_found) {
  }
};


template <class T>
OrFalse <array <var> > f$rpc_mc_multiget (const rpc_connection &conn, const array <T> &keys, double timeout, bool return_false_if_not_found, bool run_synchronously) {
  mc_method = "multiget";
  resumable_finished = true;

  array <string> query_names (array_size (keys.count(), 0, true));
  int queue_id = -1;
  int keys_n = 0;
  int first_request_id = 0;
  int bytes_sent = 0;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    const string key = f$strval (it.get_value());
    const string real_key = mc_prepare_key (key);
    int is_immediate = mc_is_immediate_query (real_key);

    f$rpc_clean();
    f$store_int (MEMCACHE_GET);
    store_string (real_key.c_str() + is_immediate, real_key.size() - is_immediate);

    int current_sent_size = real_key.size() + 32;//estimate
    bytes_sent += current_sent_size;
    if (bytes_sent >= (1 << 15) && bytes_sent > current_sent_size) {
      f$rpc_flush();
      bytes_sent = current_sent_size;
    }
    int request_id = rpc_send (conn, timeout, (bool) is_immediate);
    if (request_id > 0) {
      if (first_request_id == 0) {
        first_request_id = request_id;
      }
      if (!is_immediate) {
        queue_id = wait_queue_push_unsafe (queue_id, request_id);
        keys_n++;
      }
      query_names.push_back (key);
    } else {
      return false;
    }
  }
  if (bytes_sent > 0) {
    f$rpc_flush();
  }

  if (queue_id == -1) {
    return array <var> ();
  }

  if (run_synchronously) {
    array <var> result (array_size (0, keys_n, false));

    while (keys_n > 0) {
      int request_id = wait_queue_next_synchronously (queue_id);
      if (request_id <= 0) {
        break;
      }
      keys_n--;

      int k = (int)(request_id - first_request_id);
      php_assert ((unsigned int)k < (unsigned int)query_names.count());

      bool parse_result = f$rpc_get_and_parse (request_id, -1);
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
        php_warning ("Not all data fetched during fetch memcache.Value");
      }
    }
    return result;
  }

  return start_resumable <OrFalse <array <var> > > (new rpc_mc_multiget_resumable (queue_id, first_request_id, keys_n, query_names, return_false_if_not_found));
}
