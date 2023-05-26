// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/algorithms/hashes.h"
#include "runtime/dummy-visitor-methods.h"
#include "runtime/kphp_core.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "runtime/to-array-processor.h"

struct rpc_request {
  int64_t resumable_id; // == 0 - default, > 0 if not finished, -1 if received an answer, -2 if received an error, -3 if answer was gotten
  union {
    kphp_event_timer *timer;
    char *answer;
    struct {
      const char *error;
      int32_t error_code;
    };
  };
  uint32_t function_magic{0};
  int64_t actor_id{-1};
};

extern long long rpc_tl_results_last_query_num;

extern const string tl_str_;
extern const string tl_str_underscore;
extern const string tl_str_resultFalse;
extern const string tl_str_resultTrue;
extern const string tl_str_result;

extern const int64_t tl_str_underscore_hash;
extern const int64_t tl_str_result_hash;

extern bool fail_rpc_on_int32_overflow;

extern bool rpc_stored;

using slot_id_t = int;
rpc_request *get_rpc_request(slot_id_t request_id);

void process_rpc_answer(int32_t request_id, char *result, int32_t result_len);

void process_rpc_error(int32_t request_id, int32_t error_code, const char *error_message);

void rpc_parse_restore_previous();

const char *last_rpc_error_message_get();

int32_t last_rpc_error_code_get();

void last_rpc_error_reset();

void rpc_parse(const int32_t *new_rpc_data, int32_t new_rpc_data_len);

bool f$rpc_parse(const string &new_rpc_data);

bool f$rpc_parse(const mixed &new_rpc_data);

bool f$rpc_parse(bool new_rpc_data);

bool f$rpc_parse(const Optional<string> &new_rpc_data);

int32_t rpc_get_pos();

bool rpc_set_pos(int32_t pos);

int32_t rpc_lookup_int();

int32_t rpc_fetch_int();

int64_t f$fetch_int();

int64_t f$fetch_lookup_int();
string f$fetch_lookup_data(int64_t x4_bytes_length);

int64_t f$fetch_long();

double f$fetch_double();

double f$fetch_float();

string f$fetch_string();

int64_t f$fetch_string_as_int();

mixed f$fetch_memcache_value();

bool f$fetch_eof();

bool f$fetch_end();

void f$fetch_raw_vector_double(array<double> &out, int64_t n_elems);

void estimate_and_flush_overflow(size_t &bytes_sent);

struct tl_func_base;
using tl_storer_ptr = std::unique_ptr<tl_func_base>(*)(const mixed&);
extern array<tl_storer_ptr> tl_storers_ht;
using tl_fetch_wrapper_ptr = array<mixed>(*)(std::unique_ptr<tl_func_base>);
extern tl_fetch_wrapper_ptr tl_fetch_wrapper; // pointer to function below
//array<mixed> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base> stored_fetcher) {
//  tl_exclamation_fetch_wrapper X(std::move(stored_fetcher));
//  return t_ReqResult<tl_exclamation_fetch_wrapper, 0>(std::move(X)).fetch();
//}

inline void register_tl_storers_table_and_fetcher(const array<tl_storer_ptr> &gen$ht, tl_fetch_wrapper_ptr gen$t_ReqResult_fetch) {
  tl_storers_ht = gen$ht;
  tl_fetch_wrapper = gen$t_ReqResult_fetch;
};

struct C$RpcConnection final : public refcountable_php_classes<C$RpcConnection>, private DummyVisitorMethods {
  int32_t host_num{-1};
  int32_t port{-1};
  int32_t timeout_ms{-1};
  int64_t actor_id{-1};
  int32_t connect_timeout{-1};
  int32_t reconnect_timeout{-1};

  C$RpcConnection(int32_t host_num, int32_t port, int32_t tmeout_ms, int64_t actor_id, int32_t connect_timeout, int32_t reconnect_timeout);

  const char *get_class() const  noexcept {
    return R"(RpcConnection)";
  }

  int get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$RpcConnection::get_class())));
  }

  using DummyVisitorMethods::accept;

  void accept(ToArrayVisitor &visitor) {
    visitor("host_num", host_num);
    visitor("port", port);
    visitor("timeout_ms", timeout_ms);
    visitor("actor_id", actor_id);
    visitor("connect_timeout", connect_timeout);
    visitor("reconnect_timeout", reconnect_timeout);
  }
};

class_instance<C$RpcConnection> f$new_rpc_connection(const string &host_name, int64_t port, const mixed &actor_id = 0, double timeout = 0.3, double connect_timeout = 0.3, double reconnect_timeout = 17);

void f$store_gzip_pack_threshold(int64_t pack_threshold_bytes);

void f$store_start_gzip_pack();

void f$store_finish_gzip_pack(int64_t threshold);

bool f$store_header(const mixed &cluster_id, int64_t flags = 0);

bool f$store_error(int64_t error_code, const string &error_text);

bool f$store_raw(const string &data);

void f$store_raw_vector_double(const array<double> &vector);

bool f$set_fail_rpc_on_int32_overflow(bool fail_rpc); // TODO: remove when all RPC errors will be fixed

bool is_int32_overflow(int64_t v);
bool store_int(int32_t v);
bool f$store_int(int64_t v);

bool f$store_unsigned_int(const string &v);

bool store_long(long long v);
bool store_long(const mixed &v);

bool f$store_unsigned_long(const string &v);

bool f$store_unsigned_int_hex(const string &v);

bool f$store_unsigned_long_hex(const string &v);

bool f$store_double(double v);

bool f$store_float(double v);

bool store_string(const char *v, int32_t v_len);
bool f$store_string(const string &v);

bool f$store_many(const array<mixed> &a);

bool f$store_finish();

bool f$rpc_clean(bool is_error = false);

bool rpc_store(bool is_error = false);

int64_t f$rpc_send(const class_instance<C$RpcConnection> &conn, double timeout = -1.0);
int64_t rpc_send(const class_instance<C$RpcConnection> &conn, double timeout, bool ignore_answer = false);

int64_t f$rpc_send_noflush(const class_instance<C$RpcConnection> &conn, double timeout = -1.0);

void f$rpc_flush();

Optional<string> f$rpc_get(int64_t request_id, double timeout = -1.0);

Optional<string> f$rpc_get_synchronously(int64_t request_id);

bool rpc_get_and_parse(int64_t request_id, double timeout);
bool f$rpc_get_and_parse(int64_t request_id, double timeout = -1.0);


int64_t f$rpc_queue_create();

int64_t f$rpc_queue_create(const mixed &request_ids);

int64_t f$rpc_queue_push(int64_t queue_id, const mixed &request_ids);

bool f$rpc_queue_empty(int64_t queue_id);

Optional<int64_t> f$rpc_queue_next(int64_t queue_id, double timeout = -1);

Optional<int64_t> f$rpc_queue_next_synchronously(int64_t queue_id);

bool f$store_unsigned_int(const mixed &v);

bool f$rpc_wait(int64_t request_id);

bool f$rpc_wait_concurrently(int64_t request_id);

bool f$store_long(int64_t v);

bool f$store_unsigned_long(const mixed &v);

int32_t tl_parse_int();

long long tl_parse_long();

double tl_parse_double(); // TODO: why do these functions exist? (tl_parse_*)

double tl_parse_float();

string tl_parse_string();

int64_t f$rpc_tl_query_one(const class_instance<C$RpcConnection> &c, const mixed &tl_object, double timeout = -1.0);

int64_t f$rpc_tl_pending_queries_count();
bool f$rpc_mc_parse_raw_wildcard_with_flags_to_array(const string &raw_result, array<mixed> &result);

array<int64_t> f$rpc_tl_query(const class_instance<C$RpcConnection> &c, const array<mixed> &tl_objects, double timeout = -1.0, bool ignore_answer = false);

array<mixed> f$rpc_tl_query_result_one(int64_t query_id);

array<array<mixed>> f$rpc_tl_query_result(const array<int64_t> &query_ids);

template<class T>
array<array<mixed>> f$rpc_tl_query_result(const array<T> &query_ids);

array<array<mixed>> f$rpc_tl_query_result_synchronously(const array<int64_t> &query_ids);

template<class T>
array<array<mixed>> f$rpc_tl_query_result_synchronously(const array<T> &query_ids);

int64_t f$query_x2(int64_t x);


void global_init_rpc_lib();

void init_rpc_lib();

void free_rpc_lib();

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
array<array<mixed>> f$rpc_tl_query_result(const array<T> &query_ids) {
  return f$rpc_tl_query_result(array<int64_t>::convert_from(query_ids));
}

template<class T>
array<array<mixed>> f$rpc_tl_query_result_synchronously(const array<T> &query_ids) {
  return f$rpc_tl_query_result_synchronously(array<int64_t>::convert_from(query_ids));
}
