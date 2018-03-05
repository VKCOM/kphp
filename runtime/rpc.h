#pragma once

#include "runtime/integer_types.h"
#include "runtime/kphp_core.h"
#include "runtime/resumable.h"

void process_rpc_answer (int request_id, char *result, int result_len);

void process_rpc_error (int request_id, int error_code, const char *error_message);


void rpc_parse (const int *new_rpc_data, int new_rpc_data_len);

bool f$rpc_parse (const string &new_rpc_data);

bool f$rpc_parse (const var &new_rpc_data);

int rpc_get_pos (void);

bool rpc_set_pos (int pos);

int rpc_lookup_int();

int f$fetch_int();

UInt f$fetch_UInt();

Long f$fetch_Long();

ULong f$fetch_ULong();

var f$fetch_unsigned_int();

var f$fetch_long();

var f$fetch_unsigned_long();

string f$fetch_unsigned_int_hex();

string f$fetch_unsigned_long_hex();

string f$fetch_unsigned_int_str();

string f$fetch_unsigned_long_str();

double f$fetch_double();

string f$fetch_string();

int f$fetch_string_as_int();

var f$fetch_memcache_value();

bool f$fetch_eof();//TODO remove parameters

bool f$fetch_end();


struct rpc_connection {
  bool bool_value;
  int host_num;
  int port;
  int timeout_ms;
  long long default_actor_id;
  int connect_timeout;
  int reconnect_timeout;

  rpc_connection (void);

  rpc_connection (bool value);

  rpc_connection (bool value, int host_num, int port, int timeout_ms, long long default_actor_id, int connect_timeout, int reconnect_timeout);

  rpc_connection& operator = (bool value);
};

rpc_connection f$new_rpc_connection (string host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

bool f$boolval (const rpc_connection &my_rpc);

bool eq2 (const rpc_connection &my_rpc, bool value);

bool eq2 (bool value, const rpc_connection &my_rpc);

bool equals (bool value, const rpc_connection &my_rpc);

bool equals (const rpc_connection &my_rpc, bool value);

bool not_equals (bool value, const rpc_connection &my_rpc);

bool not_equals (const rpc_connection &my_rpc, bool value);


void f$store_gzip_pack_threshold (int pack_threshold_bytes);

void f$store_start_gzip_pack (void);

void f$store_finish_gzip_pack (int threshold);

bool f$store_header (const var &cluster_id, int flags = 0);

bool store_error (int error_code, const char *error_text);
bool f$store_error (int error_code, const string &error_text);

bool f$store_raw (const string &data);

bool f$store_int (int v);

bool f$store_UInt (UInt v);

bool f$store_Long (Long v);

bool f$store_ULong (ULong v);

bool f$store_unsigned_int (const string &v);

bool store_long (long long v);
bool f$store_long (const string &v);

bool f$store_unsigned_long (const string &v);

bool f$store_unsigned_int_hex (const string &v);

bool f$store_unsigned_long_hex (const string &v);

bool f$store_double (double v);

bool store_string (const char *v, int v_len);
bool f$store_string (const string &v);

bool f$store_many (const array <var> &a);

bool f$store_finish (void);

bool f$rpc_clean (bool is_error = false);

string f$rpc_get_contents (void);

string f$rpc_get_clean (void);

bool rpc_store (bool is_error = false);

int f$rpc_send (const rpc_connection &conn, double timeout = -1.0);
int rpc_send (const rpc_connection &conn, double timeout, bool ignore_answer = false);

int f$rpc_send_noflush (const rpc_connection &conn, double timeout = -1.0);

void f$rpc_flush (void);

OrFalse <string> f$rpc_get (int request_id, double timeout = -1.0);

bool f$rpc_get_and_parse (int request_id, double timeout = -1.0);


inline int f$rpc_queue_create (void);

inline int f$rpc_queue_create (const var &request_ids);

inline int f$rpc_queue_push (int queue_id, const var &request_ids);

inline bool f$rpc_queue_empty (int queue_id);

inline int f$rpc_queue_next (int queue_id, double timeout = -1.0);


bool f$store_unsigned_int (const var &v);

bool f$store_long (const var &v);

bool f$store_unsigned_long (const var &v);


int f$rpc_tl_query_one (const rpc_connection &c, const var &arr, double timeout = -1.0);

int f$rpc_tl_queries_count ();
bool f$rpc_mc_parse_raw_wildcard_with_flags_to_array(const string &raw_result, array< var > &result);

array <int> f$rpc_tl_query (const rpc_connection &c, const array <var> &arr, double timeout = -1.0, bool ignore_answer = false);

array <var> f$rpc_tl_query_result_one (int query_id);

array <array <var> > f$rpc_tl_query_result (const array <int> &query_ids);

template <class T>
array <array <var> > f$rpc_tl_query_result (const array <T> &query_ids);

array <array <var> > f$rpc_tl_query_result_synchronously (const array <int> &query_ids);

template <class T>
array <array <var> > f$rpc_tl_query_result_synchronously (const array <T> &query_ids);

int f$query_x2 (int x);


void rpc_init_static_once (void);

void rpc_init_static (void);

void rpc_free_static (void);




/*
 *
 *     IMPLEMENTATION
 *
 */


template <class T>
array <array <var> > f$rpc_tl_query_result (const array <T> &query_ids) {
  return f$rpc_tl_query_result (array <int> (query_ids));
}

template <class T>
array <array <var> > f$rpc_tl_query_result_synchronously (const array <T> &query_ids) {
  return f$rpc_tl_query_result_synchronously (array <int> (query_ids));
}

int f$rpc_queue_create (void) {
  return f$wait_queue_create();
}

int f$rpc_queue_create (const var &request_ids) {
  return f$wait_queue_create (request_ids);
}

int f$rpc_queue_push (int queue_id, const var &request_ids) {
  return f$wait_queue_push (queue_id, request_ids);
}

bool f$rpc_queue_empty (int queue_id) {
  return f$wait_queue_empty (queue_id);
}

int f$rpc_queue_next (int queue_id, double timeout) {
  return f$wait_queue_next (queue_id, timeout);
}


