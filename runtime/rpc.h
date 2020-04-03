#pragma once

#include <memory>

#include "runtime/integer_types.h"
#include "runtime/kphp_core.h"
#include "runtime/resumable.h"

extern const string tl_str_;
extern const string tl_str_underscore;
extern const string tl_str_resultFalse;
extern const string tl_str_resultTrue;
extern const string tl_str_result;

extern const int tl_str_underscore_hash;
extern const int tl_str_result_hash;

extern const char *new_tl_current_function_name;

void process_rpc_answer(int request_id, char *result, int result_len);

void process_rpc_error(int request_id, int error_code, const char *error_message);

void rpc_parse_restore_previous();

const char *last_rpc_error_get();

void last_rpc_error_reset();

void rpc_parse(const int *new_rpc_data, int new_rpc_data_len);

bool f$rpc_parse(const string &new_rpc_data);

bool f$rpc_parse(const var &new_rpc_data);

bool f$rpc_parse(bool new_rpc_data);

bool f$rpc_parse(const Optional<string> &new_rpc_data);

int rpc_get_pos();

bool rpc_set_pos(int pos);

int rpc_lookup_int();

int f$fetch_int();

int f$fetch_lookup_int();
string f$fetch_lookup_data(int x4_bytes_length);

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

bool f$fetch_eof();

bool f$fetch_end();

void f$fetch_raw_vector_int(array<int> &out, int n_elems);

void f$fetch_raw_vector_double(array<double> &out, int n_elems);

void estimate_and_flush_overflow(int &bytes_sent);

struct tl_func_base;
using tl_storer_ptr = std::unique_ptr<tl_func_base>(*)(const var&);
extern array<tl_storer_ptr> tl_storers_ht;
using tl_fetch_wrapper_ptr = array<var>(*)(std::unique_ptr<tl_func_base>);
extern tl_fetch_wrapper_ptr tl_fetch_wrapper; // pointer to function below
//array<var> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base> stored_fetcher) {
//  tl_exclamation_fetch_wrapper X(std::move(stored_fetcher));
//  return t_ReqResult<tl_exclamation_fetch_wrapper, 0>(std::move(X)).fetch();
//}

inline void register_tl_storers_table_and_fetcher(const array<tl_storer_ptr> &gen$ht, tl_fetch_wrapper_ptr gen$t_ReqResult_fetch) {
  tl_storers_ht = gen$ht;
  tl_fetch_wrapper = gen$t_ReqResult_fetch;
};

struct C$RpcConnection final : public refcountable_php_classes<C$RpcConnection> {
  int host_num{-1};
  int port{-1};
  int timeout_ms{-1};
  long long default_actor_id{-1};
  int connect_timeout{-1};
  int reconnect_timeout{-1};

  C$RpcConnection(int host_num, int port, int timeout_ms, long long default_actor_id, int connect_timeout, int reconnect_timeout);

  void accept(InstanceMemoryEstimateVisitor &) {}
};

class_instance<C$RpcConnection> f$new_rpc_connection(const string &host_name, int port, const var &default_actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

void f$store_gzip_pack_threshold(int pack_threshold_bytes);

void f$store_start_gzip_pack();

void f$store_finish_gzip_pack(int threshold);

bool f$store_header(const var &cluster_id, int flags = 0);

bool store_error(int error_code, const char *error_text);
bool f$store_error(int error_code, const string &error_text);

bool f$store_raw(const string &data);

void f$store_raw_vector_int(const array<int> &vector);

void f$store_raw_vector_double(const array<double> &vector);

bool f$store_int(int v);

bool f$store_UInt(UInt v);

bool f$store_Long(Long v);

bool f$store_ULong(ULong v);

bool f$store_unsigned_int(const string &v);

bool store_long(long long v);
bool f$store_long(const string &v);

bool f$store_unsigned_long(const string &v);

bool f$store_unsigned_int_hex(const string &v);

bool f$store_unsigned_long_hex(const string &v);

bool f$store_double(double v);

bool store_string(const char *v, int v_len);
bool f$store_string(const string &v);

bool f$store_many(const array<var> &a);

bool f$store_finish();

bool f$rpc_clean(bool is_error = false);

string f$rpc_get_contents();

string f$rpc_get_clean();

bool rpc_store(bool is_error = false);

int f$rpc_send(const class_instance<C$RpcConnection> &conn, double timeout = -1.0);
int rpc_send(const class_instance<C$RpcConnection> &conn, double timeout, bool ignore_answer = false);

int f$rpc_send_noflush(const class_instance<C$RpcConnection> &conn, double timeout = -1.0);

void f$rpc_flush();

Optional<string> f$rpc_get(int request_id, double timeout = -1.0);

Optional<string> f$rpc_get_synchronously(int request_id);

bool rpc_get_and_parse(int request_id, double timeout);
bool f$rpc_get_and_parse(int request_id, double timeout = -1.0);


int f$rpc_queue_create();

int f$rpc_queue_create(const var &request_ids);

int f$rpc_queue_push(int queue_id, const var &request_ids);

bool f$rpc_queue_empty(int queue_id);

Optional<int> f$rpc_queue_next(int queue_id, double timeout = -1);

Optional<int> f$rpc_queue_next_synchronously(int queue_id);

bool f$store_unsigned_int(const var &v);

bool f$rpc_wait(int request_id);

bool f$rpc_wait_multiple(int request_id);

bool f$store_long(const var &v);

bool f$store_unsigned_long(const var &v);

int tl_parse_int();

long long tl_parse_long();

double tl_parse_double();

string tl_parse_string();

int f$rpc_tl_query_one(const class_instance<C$RpcConnection> &c, const var &tl_object, double timeout = -1.0);

int f$rpc_tl_pending_queries_count();
bool f$rpc_mc_parse_raw_wildcard_with_flags_to_array(const string &raw_result, array<var> &result);

array<int> f$rpc_tl_query(const class_instance<C$RpcConnection> &c, const array<var> &tl_objects, double timeout = -1.0, bool ignore_answer = false);

array<var> f$rpc_tl_query_result_one(int query_id);

array<array<var>> f$rpc_tl_query_result(const array<int> &query_ids);

template<class T>
array<array<var>> f$rpc_tl_query_result(const array<T> &query_ids);

array<array<var>> f$rpc_tl_query_result_synchronously(const array<int> &query_ids);

template<class T>
array<array<var>> f$rpc_tl_query_result_synchronously(const array<T> &query_ids);

bool f$set_tl_mode(int mode);

int f$query_x2(int x);


void global_init_rpc_lib();

void init_rpc_lib();

void free_rpc_lib();

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
array<array<var>> f$rpc_tl_query_result(const array<T> &query_ids) {
  return f$rpc_tl_query_result(array<int>::convert_from(query_ids));
}

template<class T>
array<array<var>> f$rpc_tl_query_result_synchronously(const array<T> &query_ids) {
  return f$rpc_tl_query_result_synchronously(array<int>::convert_from(query_ids));
}
