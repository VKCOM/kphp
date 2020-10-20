#include "common/tl/parse.h"

#include <functional>
#include <memory>

#include "common/binlog/kdb-binlog-common.h"

#include "common/stats/provider.h"
#include "common/tl/constants/common.h"
#include "common/tl/methods/base.h"
#include "common/tl/methods/compression.h"
#include "common/tl/methods/network.h"
#include "common/tl/methods/rwm.h"
#include "common/tl/methods/string.h"
#include "common/tl/query-header.h"
#include "common/tl/store.h"
#include "common/wrappers/string_view.h"

static long long rpc_sent_queries;
static long long rpc_sent_answers;
static long long rpc_sent_errors;

STATS_PROVIDER(tl, 2000) {
  add_histogram_stat_long(stats, "rpc_sent_queries", rpc_sent_queries);
  add_histogram_stat_long(stats, "rpc_sent_answers", rpc_sent_answers);
  add_histogram_stat_long(stats, "rpc_sent_errors", rpc_sent_errors);
}


struct tl_state {
  std::unique_ptr<tl_in_methods> in_methods;
  std::unique_ptr<tl_out_methods> out_methods;

  int64_t in_remaining = 0;
  int64_t in_pos = 0;
  int64_t in_mark_pos = 0;

  int64_t out_pos = 0;
  int64_t out_remaining = 0;
  vk::optional<std::string> error;
  int errnum;

  int attempt_num = 0;
  long long qid = 0;
  result_header_settings header_settings;
};

thread_local std::vector<tl_state> tlio_v(1);
thread_local tl_state *tlio = &tlio_v.back();

void tl_fetch_set_error_format(int errnum, const char *format, ...) {
  if (!tlio->error) {
    assert (format);
    static char s[10000];
    va_list l;
    va_start (l, format);
    vsnprintf(s, sizeof(s) - 1, format, l);
    vkprintf(2, "Error %s\n", s);
    tl_fetch_set_error(errnum, s);
  }
}

int tl_store_stats(const char *s, int raw, const vk::optional<std::vector<std::string>> &sorted_filter_keys) {
  std::vector<std::pair<vk::string_view, vk::string_view>> stats;
  int key_start = 0;
  int value_start = -1;
  for (int i = 0; s[i]; i++) {
    if (s[i] == '\n') {
      if (value_start - key_start > 1 && value_start < i) {
        vk::string_view key(s + key_start, value_start - key_start - 1); /* - 1 (trim tabular) */
        if (!sorted_filter_keys.has_value() || std::binary_search(sorted_filter_keys->begin(), sorted_filter_keys->end(), key)) {
          vk::string_view value(s + value_start, i - value_start);
          stats.emplace_back(key, value);
        }
      }
      key_start = i + 1;
      value_start = -1;
    } else if (s[i] == '\t') {
      if (value_start == -1) {
        value_start = i + 1;
      }
    }
  }
  if (!raw) {
    tl_store_int(TL_STAT);
  }
  vk::tl::store_vector(stats, [](std::pair<vk::string_view, vk::string_view> stat) {
    vk::tl::store_string(stat.first);
    vk::tl::store_string(stat.second);
  });
  return stats.size();
}

void tl_fetch_set_error(int errnum, std::string error) {
  if (!tlio->error) {
    vkprintf(2, "Error %s\n", error.c_str());
    tlio->error = std::move(error);
    tlio->errnum = errnum;
  }
}


void tl_fetch_set_error(int errnum, const char *s) {
  assert (s);
  return tl_fetch_set_error(errnum, std::string(s));
}

void tl_fetch_init(std::unique_ptr<tl_in_methods> methods, int64_t size) {
  tlio->in_remaining = size;
  tlio->in_pos = 0;

  tlio->in_methods = std::move(methods);
  tlio->attempt_num = 0;
  tlio->error = vk::nullopt;
  tlio->errnum = 0;
}

void tl_setup_result_header(const tl_query_header_t *header) {
  tlio->header_settings.header_flags = header->flags;
  tlio->header_settings.compression_version = header->supported_compression_version;
  // TODO: use another?
  if (tlio->header_settings.compression_version > COMPRESSION_VERSION_MAX) {
    tlio->header_settings.compression_version = COMPRESSION_VERSION_MAX;
  }
}


void tl_store_init(std::unique_ptr<tl_out_methods> methods, int64_t size) {
  tlio->out_methods = std::move(methods);
  memset(&tlio->header_settings, 0, sizeof(tlio->header_settings));

  tl_fetch_reset_error();
  tlio->out_pos = 0;
  tlio->out_remaining = size;
}


static int tl_store_end_impl(int op, bool noheader) {
  auto *out_methods = dynamic_cast<tl_out_methods_network*>(tlio->out_methods.get());
  if (!out_methods) {
    return 0;
  }
  if (!noheader) {
    tl_query_answer_header_t header;
    if (tlio->error) {
      tlio->out_remaining += tlio->out_pos;
      tlio->out_pos = 0;
      out_methods->store_reset();
      vkprintf(1, "tl_store_end: writing error %s, errnum %d, tl.out_pos = %ld\n", tlio->error->c_str(), tlio->errnum, tlio->out_pos);
      header.type = result_header_type::wrapped_error;
      header.error_code = tlio->errnum;
      header.error = *tlio->error;
      rpc_sent_errors++;
    } else {
      if (op == TL_RPC_REQ_RESULT) {
        rpc_sent_answers++;
      } else {
        rpc_sent_queries++;
      }
      set_result_header_values(&header, tlio->header_settings.header_flags);
      if (tlio->header_settings.compression_version != COMPRESSION_VERSION_NONE) {
        if (tlio->out_pos >= 256) {
          header.flags |= vk::tl::common::rpc_req_result_extra_flags::compression_version;
          header.compression_version = tlio->header_settings.compression_version;
          tl_compress_written(header.compression_version);
        }
      }
    }
    assert (!(tlio->out_pos & 3));
    static thread_local char buffer[1000000];
    int size = vk::tl::save_to_buffer(buffer, sizeof(buffer), [op, &header, qid = tlio->qid] {
      tl_store_int(op);
      tl_store_long(qid);
      tl_store_answer_header(&header);
    });

    out_methods->store_flush(buffer, size);
  } else {
    assert(!tl_fetch_error());
    out_methods->store_flush("", 0);
  }
  tlio->out_methods = nullptr;
  return 0;
}

void tl_store_raw_msg(struct raw_message *raw, int dup) {
  assert (tl_store_check(raw->total_bytes) >= 0);
  int len = raw->total_bytes;
  raw_message rwm;
  if (dup) {
    rwm_clone(&rwm, raw);
  } else {
    rwm_steal(&rwm, raw);
  }
  tl_in_methods_raw_msg methods(&rwm);
  tlio->out_methods->copy_through(&methods, len, 1);
  tlio->out_pos += len;
  tlio->out_remaining -= len;
}

void tl_store_raw_msg(const struct raw_message &raw) {
  raw_message_t r;
  rwm_clone(&r, &raw);
  tl_store_raw_msg(&r, false);
}

void tlio_push(void) {
  assert(tlio_v.size() <= 10);
  tlio_v.emplace_back();
  tlio = &tlio_v.back();
}

void tlio_pop(void) {
  tlio_v.pop_back();
  tlio = &tlio_v.back();
}

void tlio_pop_forward_error(void) {
  if (tl_fetch_error()) {
    int error_code = tlio->errnum;
    std::string error_string = std::move(*tlio->error);
    tlio_pop();
    tl_fetch_set_error(error_code, std::move(error_string));
  } else {
    tlio_pop();
  }
}



void tl_compress_written(int version) {
  auto *methods = dynamic_cast<tl_out_methods_network*>(tlio->out_methods.get());
  assert(methods);
  int new_size = methods->compress(version);
  assert(new_size % 4 == 0);
  tlio->out_remaining += new_size - tlio->out_pos;
  tlio->out_pos = new_size;
}

void tl_decompress_remaining(int version) {
  int new_size = tlio->in_methods->decompress(version);
  assert(new_size % 4 == 0);
  tlio->in_remaining = new_size;
}

void tl_fetch_raw_data(void *buf, int size) {
  tlio->in_methods->fetch_raw_data(buf, size);
  tlio->in_pos += size;
  tlio->in_remaining -= size;
}

void tl_fetch_chunked(int size, const std::function<void(const void*, int)> &callback) {
  if (tl_fetch_check(size) < 0) {
    return;
  }
  tlio->in_methods->fetch_chunked(size, callback);
  tlio->in_pos += size;
  tlio->in_remaining -= size;
}

void tl_fetch_lookup_chunked(int size, const std::function<void(const void*, int)> &callback) {
  if (tl_fetch_check(size) < 0) {
    return;
  }
  tlio->in_methods->fetch_lookup_chunked(size, callback);
}

void tl_fetch_skip_raw_data(int size) {
  tlio->in_methods->fetch_move(size);
  tlio->in_pos += size;
  tlio->in_remaining -= size;
}

int tl_fetch_lookup_int(void) {
  if (tl_fetch_check(4) < 0) {
    return -1;
  }
  int x;
  tlio->in_methods->fetch_lookup(&x, 4);
  return x;
}

int tl_fetch_lookup_data(char *data, int len) {
  if (tl_fetch_check(len) < 0) {
    return -1;
  }
  tlio->in_methods->fetch_lookup(data, len);
  return len;
}

void tl_fetch_mark(void) {
  tlio->in_methods->fetch_mark();
  tlio->in_mark_pos = tlio->in_pos;
}

void tl_fetch_mark_restore(void) {
  tlio->in_methods->fetch_mark_restore();
  int64_t x = tlio->in_pos - tlio->in_mark_pos;
  tlio->in_pos -= x;
  tlio->in_remaining += x;
}

void tl_fetch_mark_delete(void) {
  tlio->in_methods->fetch_mark_delete();
}

int tl_fetch_string_len(int max_len) {
  if (tl_fetch_check(4) < 0) {
    return -1;
  }
  int x = 0;
  tl_fetch_raw_data(&x, 1);
  if (x == 255) {
    tl_fetch_set_error(TL_ERROR_SYNTAX, "String len can not start with 0xff");
    return -1;
  }
  if (x == 254) {
    tl_fetch_raw_data(&x, 3);
  }
  if (x > max_len) {
    tl_fetch_set_error_format(TL_ERROR_TOO_LONG_STRING, "string is too long: max_len = %d, len = %d", max_len, x);
    return -1;
  }
  if (x > tlio->in_remaining) {
    tl_fetch_set_error_format(TL_ERROR_SYNTAX, "string is too long: remaining_bytes = %ld, len = %d", tlio->in_remaining, x);
    return -1;
  }
  return x;
}

int tl_fetch_pad(void) {
  if (tl_fetch_check(0) < 0) {
    return -1;
  }
  int t = 0;
  int pad = (-tlio->in_pos) & 3;
  assert(tlio->in_remaining >= pad);
  tl_fetch_raw_data(&t, pad);
  if (t) {
    tl_fetch_set_error(TL_ERROR_SYNTAX, "Padding with non-zeroes");
    return -1;
  }
  return pad;
}

int tl_fetch_data(void *buf, int len) {
  if (tl_fetch_check(len) < 0) {
    return -1;
  }
  tl_fetch_raw_data(buf, len);
  return len;
}

int tl_fetch_string_data(char *buf, int len) {
  if (tl_fetch_check(len) < 0) {
    return -1;
  }
  tl_fetch_raw_data(buf, len);
  if (tl_fetch_pad() < 0) {
    return -1;
  }
  return len;
}

int tl_fetch_skip_string_data(int len) {
  if (tl_fetch_check(len) < 0) {
    return -1;
  }
  tl_fetch_skip_raw_data(len);
  if (tl_fetch_pad() < 0) {
    return -1;
  }
  return len;
}

int tl_fetch_skip_string(int max_len) {
  int l = tl_fetch_string_len(max_len);
  if (l < 0) {
    return -1;
  }
  return tl_fetch_skip_string_data(l);
}

int tl_fetch_string(char *buf, int max_len) {
  int l = tl_fetch_string_len(max_len);
  if (l < 0) {
    return -1;
  }
  if (tl_fetch_string_data(buf, l) < 0) {
    return -1;
  }
  return l;
}

int tl_fetch_string0(char *buf, int max_len) {
  int l = tl_fetch_string_len(max_len);
  if (l < 0) {
    return -1;
  }
  if (tl_fetch_string_data(buf, l) < 0) {
    return -1;
  }
  buf[l] = 0;
  return l;
}

void tl_store_raw_data_nopad(const void *buf, int len) {
  tlio->out_methods->store_raw_data(buf, len);
  tlio->out_pos += len;
  tlio->out_remaining -= len;
}

void *tl_store_get_ptr(int size) {
  assert (tl_store_check(size) >= 0);
  if (!size) {
    return 0;
  }
  assert (size >= 0);
  void *x = tlio->out_methods->store_get_ptr(size);
  tlio->out_pos += size;
  tlio->out_remaining -= size;
  return x;
}

int tl_store_clear(void) {
  tlio->out_methods = nullptr;
  return 0;
}

int tl_fetch_int(void) {
  if (__builtin_expect(tl_fetch_check(4) < 0, 0)) {
    return -1;
  }
  int x;
  tl_fetch_raw_data(&x, 4);
  return x;
}

bool tl_fetch_bool(void) {
  int x = tl_fetch_int();
  if (x == TL_BOOL_TRUE) {
    return true;
  }
  if (x == TL_BOOL_FALSE) {
    return false;
  }
  tl_fetch_set_error_format(TL_ERROR_BAD_VALUE, "Expected bool, but 0x%08x found", x);
  return false;
}

bool tl_fetch_maybe_magic() {
  int x = tl_fetch_int();
  if (x == TL_MAYBE_TRUE) {
    return true;
  }
  if (x == TL_MAYBE_FALSE) {
    return false;
  }
  tl_fetch_set_error_format(TL_ERROR_BAD_VALUE, "Expected Maybe, but 0x%08x found", x);
  return false;
}

double tl_fetch_double(void) {
  if (__builtin_expect(tl_fetch_check(sizeof(double)) < 0, 0)) {
    return -1;
  }
  double x;
  tl_fetch_raw_data(&x, sizeof(x));
  return x;
}

double tl_fetch_double_in_range(double min, double max) {
  double res = tl_fetch_double();
  const double eps = 1e-9;
  if (!(min - eps <= res && res <= max + eps)) {
    tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "Expected double in range [%.15f,%.15f], %.f presented", min, max, res);
  }
  return res;
}

float tl_fetch_float(void) {
  if (__builtin_expect(tl_fetch_check(sizeof(float)) < 0, 0)) {
    return -1;
  }
  float x;
  tl_fetch_raw_data(&x, sizeof(x));
  return x;
}

float tl_fetch_float_in_range(float min, float max) {
  float res = tl_fetch_float();
  const float eps = 1e-9f;
  if (!(min - eps <= res && res <= max + eps)) {
    tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "Expected float in range [%.15f,%.15f], %.f presented", min, max, res);
  }
  return res;
}

long long tl_fetch_long(void) {
  if (__builtin_expect(tl_fetch_check(8) < 0, 0)) {
    return -1;
  }
  long long x;
  tl_fetch_raw_data(&x, 8);
  return x;
}

void tl_store_bool(bool x) {
  tl_store_int(x ? TL_BOOL_TRUE : TL_BOOL_FALSE);
}

void tl_store_bool_stat_bare(int x) {
  tl_store_int(x ? 1 : 0);
  tl_store_int(x ? 0 : 1);
  tl_store_int(0);
}

void tl_store_bool_stat(int x) {
  tl_store_int(TL_BOOL_STAT);
  tl_store_bool_stat_bare(x);
}

void tl_store_bool_stat_full_bare(int x, int y, int z) {
  tl_store_int(x);
  tl_store_int(y);
  tl_store_int(z);
}

void tl_store_bool_stat_full(int x, int y, int z) {
  tl_store_int(TL_BOOL_STAT);
  tl_store_bool_stat_full_bare(x, y, z);
}

int tl_store_end_noheader() {
  return tl_store_end_impl(0, true);
}

int tl_store_end(void) {
  return tl_store_end_impl(TL_RPC_REQ_RESULT, false);
}

void tl_store_invoke_req(long long qid) {
  tl_store_int(TL_RPC_INVOKE_REQ);
  tl_store_long(qid);
}

int tl_store_empty_vector() {
  tl_store_int(TL_VECTOR);
  tl_store_int(0);
  return 0;
}

void tl_store_int_forward(long long forward_id) {
  tl_store_int(TL_RPC_DEST_FLAGS);
  tl_store_int(vk::tl::common::rpc_invoke_req_extra_flags::int_forward);
  tl_store_long(forward_id);
}

int tl_store_pad() {
  assert (tl_store_check(0) >= 0);
  int x = 0;
  int pad = (-tlio->out_pos) & 3;
  tl_store_raw_data_nopad(&x, pad);
  return 0;
}

static int get_tl_string_len(int len) {
  return (len << 8) + 0xfe;
}

static int tl_store_string_len(int len) {
  assert (len >= 0);
  if (len < 254) {
    assert(tl_store_check(1) >= 0);
    tl_store_raw_data_nopad(&len, 1);
  } else {
    assert(len < (1 << 24));
    tl_store_int(get_tl_string_len(len));
  }
  return 0;
}

void tl_store_string_hex(const unsigned char *s, int len) {
  tl_store_string_len(2 * len);
  for (int i = 0; i < len; i++) {
    char data[3];
    sprintf(data, "%02x", s[i]);
    tl_store_raw_data_nopad(data, 2);
  }
  tl_store_pad();
}

void tl_store_string(const char *s, int len) {
  tl_store_string_len(len);
  tl_store_raw_data(s, len);
}

void tl_store_raw_data(const void *s, int len) {
  assert (tl_store_check(len) >= 0);
  tl_store_raw_data_nopad(s, len);
  tl_store_pad();
}

void tl_store_int(int x) {
  assert (tl_store_check(4) >= 0);
  tl_store_raw_data_nopad(&x, 4);
}

void tl_store_long(long long x) {
  assert (tl_store_check(8) >= 0);
  tl_store_raw_data_nopad(&x, 8);
}

void tl_store_double(double x) {
  assert (tl_store_check(8) >= 0);
  tl_store_raw_data_nopad(&x, 8);
}

void tl_store_float(float x) {
  assert (tl_store_check(sizeof(x)) >= 0);
  tl_store_raw_data_nopad(&x, sizeof(x));
}


void tl_copy_through(int len, int advance) {
  if (tl_fetch_check(len) < 0 || tl_store_check(len) < 0) {
    return ;
  }
  tlio->out_methods->copy_through(tlio->in_methods.get(), len, advance);
  if (advance) {
    tlio->in_pos += len;
    tlio->in_remaining -= len;
  }
  tlio->out_pos += len;
  tlio->out_remaining -= len;
}

int tl_fetch_attempt_num() {
  return tlio->attempt_num;
}

int tl_set_attempt_num(int attempt) {
  return tlio->attempt_num = attempt;
}

int tl_store_check(int size) {
  if (!tl_is_store_inited()) {
    return -1;
  }
  if (tlio->out_remaining < size) {
    return -1;
  }
  return 0;
}

int tl_fetch_skip(int len) {
  if (tl_fetch_check(len) < 0) {
    return -1;
  }
  tl_fetch_skip_raw_data(len);
  return len;
}

int64_t tl_fetch_unread(void) {
  return tlio->in_remaining;
}

void tl_fetch_reset_error(void) {
  tlio->error = vk::nullopt;
  tlio->errnum = 0;
}

int tl_fetch_end(void) {
  if (tlio->in_remaining) {
    tl_fetch_set_error_format(TL_ERROR_EXTRA_DATA, "extra %ld bytes after query", tlio->in_remaining);
    return -1;
  }
  return 1;
}

int tl_fetch_check_eof(void) {
  return !tlio->in_remaining;
}

bool tl_fetch_error() {
  return tlio->error.has_value();
}

int tl_fetch_error_code() {
  return tlio->errnum;
}

const char* tl_fetch_error_string() {
  return tlio->error.has_value() ? tlio->error->c_str() : nullptr;
}


int tl_fetch_check(int nbytes) {
  if (!tl_is_fetch_inited()) {
    tl_fetch_set_error(TL_ERROR_INTERNAL, "Trying to read from unitialized in buffer");
    return -1;
  }
  if (nbytes >= 0) {
    if (tlio->in_remaining < nbytes) {
      tl_fetch_set_error_format(TL_ERROR_SYNTAX, "Trying to read %d bytes at position %ld (size = %ld)", nbytes, tlio->in_pos, tlio->in_pos + tlio->in_remaining);
      return -1;
    }
  } else {
    if (tlio->in_pos < -nbytes) {
      tl_fetch_set_error_format(TL_ERROR_SYNTAX, "Trying to read %d bytes at position %ld (size = %ld)", nbytes, tlio->in_pos, tlio->in_pos + tlio->in_remaining);
      return -1;
    }
  }
  if (tlio->error) {
    return -1;
  }
  return 0;
}

int64_t tl_bytes_fetched() {
  return tlio->in_pos;
}

int64_t tl_bytes_stored() {
  return tlio->out_pos;
}

bool tl_is_store_inited() {
  return tlio->out_methods != nullptr;
}

bool tl_is_fetch_inited() {
  return tlio->in_methods != nullptr;
}

long long tl_current_query_id() {
  return tlio->qid;
}

void tl_set_current_query_id(long long qid) {
  tlio->qid = qid;
}


const process_id* tl_current_query_sender() {
  auto *network_methods = dynamic_cast<tl_out_methods_network*>(tlio->out_methods.get());
  assert(network_methods);
  return network_methods->get_pid();
}


long long tl_fetch_nonnegative_long(void) {
  return tl_fetch_long_range(0, std::numeric_limits<long long>::max());
}

long long tl_fetch_positive_long(void) {
  return tl_fetch_long_range(1, std::numeric_limits<long long>::max());
}

long long tl_fetch_long_range(long long int min, long long int max) {
  long long x = tl_fetch_long();
  if (x < min || x > max) {
    tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "Expected int64 in range [%lld,%lld], %lld presented", min, max, x);
  }
  return x;
}

int tl_fetch_nonnegative_int(void) {
  return tl_fetch_int_range(0, std::numeric_limits<int>::max());
}

int tl_fetch_positive_int(void) {
  return tl_fetch_int_range (1, std::numeric_limits<int>::max());
}

int tl_fetch_int_range_with_error(int min, int max, const char *error) {
  int x = tl_fetch_int ();
  if (x < min || x > max) {
    tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "%s. Expected int32 in range [%d,%d], %d presented", error, min, max, x);
    return -1;
  }
  return x;
}

int tl_fetch_int_range(int min, int max) {
  int x = tl_fetch_int();
  if (x < min || x > max) {
    tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "Expected int32 in range [%d,%d], %d presented", min, max, x);
  }
  return x;
}

void tl_store_string0(const char *s) {
  if (!s) {
    return tl_store_string(nullptr, 0);
  }
  return tl_store_string(s, strlen(s));
}
