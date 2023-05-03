// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "common/wrappers/string_view.h"

#include "runtime/kphp_core.h"
#include "runtime/optional.h"
#include "server/php-query-data.h"

extern string_buffer *coub;//TODO static
using shutdown_function_type = std::function<void()>;

enum class shutdown_functions_status {
  not_executed,
  running,
  running_from_timeout,
};

void f$ob_clean();

bool f$ob_end_clean();

Optional<string> f$ob_get_clean();

string f$ob_get_contents();

void f$ob_start(const string &callback = string());

void f$ob_flush();

bool f$ob_end_flush();

Optional<string> f$ob_get_flush();

Optional<int64_t> f$ob_get_length();

int64_t f$ob_get_level();

void f$flush();

void f$header(const string &str, bool replace = true, int64_t http_response_code = 0);

array<string> f$headers_list();

void f$send_http_103_early_hints(const array<string> & headers);

void f$setcookie(const string &name, const string &value, int64_t expire = 0, const string &path = string(), const string &domain = string(), bool secure = false, bool http_only = false);

void f$setrawcookie(const string &name, const string &value, int64_t expire = 0, const string &path = string(), const string &domain = string(), bool secure = false, bool http_only = false);

int64_t f$ignore_user_abort(Optional<bool> enable = Optional<bool>());

void run_shutdown_functions_from_timeout();
void run_shutdown_functions_from_script();

int get_shutdown_functions_count();
shutdown_functions_status get_shutdown_functions_status();

void f$register_shutdown_function(const shutdown_function_type &f);

bool f$set_wait_all_forks_on_finish(bool wait = true) noexcept;

void f$fastcgi_finish_request(int64_t exit_code = 0);

__attribute__((noreturn))
void finish(int64_t exit_code, bool allow_forks_waiting);

__attribute__((noreturn))
void f$exit(const mixed &v = 0);

__attribute__((noreturn))
void f$die(const mixed &v = 0);

Optional<int64_t> f$ip2long(const string &ip);

Optional<string> f$ip2ulong(const string &ip);

string f$long2ip(int64_t num);

Optional<array<string>> f$gethostbynamel(const string &name);

Optional<string> f$inet_pton(const string &address);


void print(const char *s, size_t s_len);

void print(const char *s);

void print(const string &s);

void print(const string_buffer &sb);

void dbg_echo(const char *s, size_t s_len);

void dbg_echo(const char *s);

void dbg_echo(const string &s);

void dbg_echo(const string_buffer &sb);

inline int64_t f$print(const string& s) {
  print(s);
  return 1;
}

inline void f$echo(const string& s) {
  print(s);
}

inline void f$dbg_echo(const string& s) {
  dbg_echo(s);
}



bool f$get_magic_quotes_gpc();


string f$php_sapi_name();

extern mixed runtime_config;

mixed f$kphp_get_runtime_config();

extern mixed v$_SERVER;
extern mixed v$_GET;
extern mixed v$_POST;
extern mixed v$_FILES;
extern mixed v$_COOKIE;
extern mixed v$_REQUEST;
extern mixed v$_ENV;

const int32_t UPLOAD_ERR_OK = 0;
const int32_t UPLOAD_ERR_INI_SIZE = 1;
const int32_t UPLOAD_ERR_FORM_SIZE = 2;
const int32_t UPLOAD_ERR_PARTIAL = 3;
const int32_t UPLOAD_ERR_NO_FILE = 4;
const int32_t UPLOAD_ERR_NO_TMP_DIR = 6;
const int32_t UPLOAD_ERR_CANT_WRITE = 7;
const int32_t UPLOAD_ERR_EXTENSION = 8;


bool f$is_uploaded_file(const string &filename);

bool f$move_uploaded_file(const string &oldname, const string &newname);


void init_superglobals(php_query_data *data);


double f$get_net_time();

double f$get_script_time();

int64_t f$get_net_queries_count();


int64_t f$get_engine_uptime();

string f$get_engine_version();

int64_t f$get_engine_workers_number();

string f$get_kphp_cluster_name();

std::tuple<int64_t, int64_t, int64_t, int64_t> f$get_webserver_stats();

void arg_add(const char *value);

void ini_set(vk::string_view key, vk::string_view value);
int32_t ini_set_from_config(const char *config_file_name);

void read_engine_tag(const char *file_name);

bool f$ini_set(const string &s, const string &value);

Optional<string> f$ini_get(const string &s);

Optional<int64_t> &get_dummy_rest_index() noexcept;
Optional<array<mixed>> f$getopt(const string &options, array<string> longopts = {}, Optional<int64_t> &rest_index = get_dummy_rest_index());

void global_init_runtime_libs();
void global_init_script_allocator();

void init_runtime_environment(php_query_data *data, void *mem, size_t script_mem_size, size_t oom_handling_mem_size = 0);

void free_runtime_environment();

// called only once at the beginning of each worker
void worker_global_init() noexcept;

void use_utf8();

/*
 *
 *     IMPLEMENTATION
 *
 */

// for degug use only
void f$raise_sigsegv();

template<class T>
T f$make_clone(const T &x) {
  return x;
}

extern bool is_json_log_on_timeout_enabled;

inline void f$set_json_log_on_timeout_mode(bool enabled) {
  is_json_log_on_timeout_enabled = enabled;
}

int64_t f$numa_get_bound_node();

bool f$extension_loaded(const string &extension);
