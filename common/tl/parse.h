#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/pid.h"
#include "common/rpc-error-codes.h"
#include "common/wrappers/optional.h"
#include "common/tl/methods/base.h"


struct tl_query_header_t;

struct result_header_settings {
  int header_flags;
  int compression_version;
};


struct connection;
struct raw_message;

void tl_fetch_set_error_format(int errnum, const char *format, ...) __attribute__ (( format(printf, 2, 3)));
void tl_fetch_set_error(int errnum, const char *s);

int tl_store_end_noheader();
void tl_store_invoke_req(long long qid);

void tl_setup_result_header(const struct tl_query_header_t *header);

void tl_compress_written(int version);
void tl_decompress_remaining(int version);

int tl_fetch_check(int nbytes);

void tl_fetch_raw_data(void *buf, int size);
void tl_fetch_skip_raw_data(int size);
int tl_fetch_lookup_int(void);
int tl_fetch_lookup_data(char *data, int len);

int tl_fetch_int(void);
bool tl_fetch_bool(void);
bool tl_fetch_maybe_magic();
double tl_fetch_double(void);
double tl_fetch_double_in_range(double min, double max);
float tl_fetch_float();
float tl_fetch_float_in_range(float min, float max);
long long tl_fetch_long(void);

void tl_fetch_mark(void);
void tl_fetch_mark_restore(void);
void tl_fetch_mark_delete(void);


int tl_fetch_string_len(int max_len);
int tl_fetch_pad(void);
int tl_fetch_data(void *buf, int len);
int tl_fetch_string_data(char *buf, int len);
int tl_fetch_skip_string_data(int len);
int tl_fetch_skip_string(int max_len);
int tl_fetch_string(char *buf, int max_len);
int tl_fetch_string0(char *buf, int max_len);

bool tl_fetch_error(void);
int tl_fetch_error_code(void);
const char* tl_fetch_error_string(void);

int tl_fetch_check_eof(void);

int tl_fetch_end(void);

void tl_fetch_reset_error(void);

int64_t tl_fetch_unread(void);
int64_t tl_bytes_fetched();
int64_t tl_bytes_stored();

int tl_fetch_attempt_num(void);
int tl_set_attempt_num(int attempt);

int tl_fetch_skip(int len);
int tl_store_check(int size);

void *tl_store_get_ptr(int size);

void tl_store_int(int x);
void tl_store_long(long long x);
void tl_store_double(double x);
void tl_store_float(float x);

void tl_store_bool(bool x);

void tl_store_bool_stat_bare(int x);
void tl_store_bool_stat(int x);
void tl_store_bool_stat_full_bare(int x, int y, int z);
void tl_store_bool_stat_full(int x, int y, int z);

void tl_store_raw_msg(struct raw_message *raw, int dup);
void tl_store_raw_data(const void *s, int len);
void tl_store_raw_data_nopad(const void *s, int len);
int tl_store_pad();
void  tl_store_string(const char *s, int len);
void tl_store_string_hex(const unsigned char *s, int len);
void tl_store_string0(const char *s);
int tl_store_clear(void);
int tl_store_end(void);
int tl_store_empty_vector();
void tl_copy_through(int len, int advance);

int tl_fetch_int_range(int min, int max);
int tl_fetch_int_range_with_error (int min, int max, const char *error);
int tl_fetch_positive_int(void);
int tl_fetch_nonnegative_int(void);
long long tl_fetch_long_range(long long min, long long max);
long long tl_fetch_positive_long(void);
long long tl_fetch_nonnegative_long(void);

void tlio_push(void);
void tlio_pop(void);
void tlio_pop_forward_error(void);

bool tl_is_store_inited();
bool tl_is_fetch_inited();

void tl_store_int_forward(long long forward_id);

void tl_set_current_query_id(long long qid);
long long tl_current_query_id();
const struct process_id* tl_current_query_sender();

void tl_store_raw_msg(const struct raw_message &raw);

void tl_fetch_chunked(int size, const std::function<void(const void*, int)> &callback);
void tl_fetch_lookup_chunked(int size, const std::function<void(const void*, int)> &callback);
int tl_store_stats(const char *s, int raw, const vk::optional<std::vector<std::string>> &sorted_filter_keys);
void tl_fetch_init(std::unique_ptr<tl_in_methods> methods, int64_t size);
void tl_store_init(std::unique_ptr<tl_out_methods> methods, int64_t size);
void tl_fetch_set_error(int errnum, std::string error);

namespace vk {
namespace tl {
class push_guard : vk::not_copyable {
  bool forward_error = false;
public:
  void forward_error_on_finish() {
    forward_error = true;
  }
  push_guard() { tlio_push(); }
  ~push_guard() {
    if (forward_error) {
      tlio_pop_forward_error();
    } else {
      tlio_pop();
    }
  }
};
} // namespace tl
} // namespace vk
