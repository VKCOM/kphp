#pragma once

#include <functional>

#include "PHP/common-net-functions.h"
#include "runtime/kphp_core.h"

extern string_buffer *coub;//TODO static
using shutdown_function_type = std::function<var()>;


void f$ob_clean();

bool f$ob_end_clean();

OrFalse<string> f$ob_get_clean();

string f$ob_get_contents();

void f$ob_start(const string &callback = string());

void f$ob_flush();

bool f$ob_end_flush();

OrFalse<string> f$ob_get_flush();

OrFalse<int> f$ob_get_length();

int f$ob_get_level();

void f$header(const string &str, bool replace = true, int http_response_code = 0);

void f$setcookie(const string &name, const string &value, int expire = 0, const string &path = string(), const string &domain = string(), bool secure = false, bool http_only = false);

void f$setrawcookie(const string &name, const string &value, int expire = 0, const string &path = string(), const string &domain = string(), bool secure = false, bool http_only = false);

void f$register_shutdown_function(const shutdown_function_type &callback);

void f$fastcgi_finish_request(int exit_code = 0);

void finish(int exit_code);

bool f$exit(const var &v = 0);

bool f$die(const var &v = 0);


OrFalse<int> f$ip2long(const string &ip);

OrFalse<string> f$ip2ulong(const string &ip);

string f$long2ip(int num);

template<class T>
inline string f$long2ip(const T &v);//shut up warning on converting to int

OrFalse<array<string>> f$gethostbynamel(const string &name);

OrFalse<string> f$inet_pton(const string &address);


int print(const char *s);

int print(const char *s, int s_len);

int print(const string &s);

int print(const string_buffer &sb);

int dbg_echo(const char *s);

int dbg_echo(const char *s, int s_len);

int dbg_echo(const string &s);

int dbg_echo(const string_buffer &sb);


bool f$get_magic_quotes_gpc();


string f$php_sapi_name();


extern var v$_SERVER;
extern var v$_GET;
extern var v$_POST;
extern var v$_FILES;
extern var v$_COOKIE;
extern var v$_REQUEST;
extern var v$_ENV;

const int UPLOAD_ERR_OK = 0;
const int UPLOAD_ERR_INI_SIZE = 1;
const int UPLOAD_ERR_FORM_SIZE = 2;
const int UPLOAD_ERR_PARTIAL = 3;
const int UPLOAD_ERR_NO_FILE = 4;
const int UPLOAD_ERR_NO_TMP_DIR = 6;
const int UPLOAD_ERR_CANT_WRITE = 7;
const int UPLOAD_ERR_EXTENSION = 8;


bool f$is_uploaded_file(const string &filename);

bool f$move_uploaded_file(const string &oldname, const string &newname);

void f$parse_multipart(const string &post, const string &boundary);


void init_superglobals(php_query_data *data);


bool f$set_server_status(const string &status);


double f$get_net_time();

double f$get_script_time();

int f$get_net_queries_count();


int f$get_engine_uptime();

string f$get_engine_version();

int f$get_engine_workers_number();

extern "C" {
void arg_add(const char *value);

void ini_set(const char *key, const char *value);

void read_engine_tag(const char *file_name);
}

bool f$ini_set(const string &s, const string &value);

OrFalse<string> f$ini_get(const string &s);

OrFalse<array<var>> f$getopt(const string &options, array<string> longopts = array<string>());

void init_static_once();

void init_static();

void free_static();

/*
 *
 *     IMPLEMENTATION
 *
 */

template<class T>
string f$long2ip(const T &v) {
  return f$long2ip(f$intval(v));
}

// for degug use only
void f$raise_sigsegv();
