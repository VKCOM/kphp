// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/pid.h"
#include "common/rpc-error-codes.h"
#include "common/tl/methods/base.h"

struct tl_query_header_t;

struct result_header_settings {
  int header_flags;
  int compression_version;
};

struct connection;
struct raw_message;

void tl_fetch_set_error_format(int errnum, const char* format, ...) __attribute__((format(printf, 2, 3)));
void tl_fetch_set_error(int errnum, const char* s);

void tl_setup_result_header(const struct tl_query_header_t* header);

void tl_compress_written(int version);
void tl_decompress_remaining(int version);

int tl_fetch_check(int nbytes);

void tl_fetch_raw_data(void* buf, int size);
int tl_fetch_lookup_int();
int tl_fetch_lookup_data(char* data, int len);

int tl_fetch_int();
int tl_fetch_byte();
bool tl_fetch_bool();
double tl_fetch_double();
double tl_fetch_double_in_range(double min, double max);
float tl_fetch_float();
long long tl_fetch_long();

void tl_fetch_mark();
void tl_fetch_mark_restore();
void tl_fetch_mark_delete();

int tl_fetch_string_len(int max_len);
int tl_fetch_pad();
int tl_fetch_data(void* buf, int len);
int tl_fetch_string_data(char* buf, int len);
int tl_fetch_string(char* buf, int max_len);
int tl_fetch_string0(char* buf, int max_len);

bool tl_fetch_error();
int tl_fetch_error_code();
const char* tl_fetch_error_string();

int tl_fetch_end();

void tl_fetch_reset_error();

int64_t tl_fetch_unread();
int64_t tl_bytes_fetched();
int64_t tl_bytes_stored();

int tl_store_check(int size);

void* tl_store_get_ptr(int size);

void tl_store_int(int x);
void tl_store_byte(int x);
void tl_store_long(long long x);
void tl_store_double(double x);
void tl_store_float(float x);

void tl_store_bool(bool x);

void tl_store_bool_stat_bare(int x);
void tl_store_bool_stat(int x);

void tl_store_raw_data(const void* s, int len);
void tl_store_raw_data_nopad(const void* s, int len);
int tl_store_pad();
void tl_store_string(const char* s, int len);
void tl_store_string0(const char* s);
int tl_store_clear();
int tl_store_end();

int tl_fetch_int_range(int min, int max);

void tlio_push();
void tlio_pop();

bool tl_is_store_inited();
bool tl_is_fetch_inited();

void tl_set_current_query_id(long long qid);
long long tl_current_query_id();

int tl_store_stats(const char* s, int raw, const std::optional<std::vector<std::string>>& sorted_filter_keys);
void tl_fetch_init(std::unique_ptr<tl_in_methods> methods, int64_t size);
void tl_store_init(std::unique_ptr<tl_out_methods> methods, int64_t size);
void tl_fetch_set_error(int errnum, std::string error);
