// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/rpc.h"

#include <cstdarg>

#include "common/rpc-error-codes.h"
#include "common/rpc-headers.h"
#include "common/tl/constants/common.h"

#include "runtime/critical_section.h"
#include "runtime/exception.h"
#include "runtime/memcache.h"
#include "runtime/misc.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "runtime/string_functions.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_request.h"
#include "runtime/tl/rpc_server.h"
#include "runtime/tl/rpc_tl_query.h"
#include "runtime/tl/tl_builtins.h"
#include "runtime/zlib.h"
#include "server/php-queries.h"

static const int GZIP_PACKED = 0x3072cfa1;

const string tl_str_("");
const string tl_str_underscore("_");
const string tl_str_resultFalse("resultFalse");
const string tl_str_resultTrue("resultTrue");
const string tl_str_result("result");

const int64_t tl_str_underscore_hash = string_hash("_", 1);
const int64_t tl_str_result_hash = string_hash("result", 6);

bool fail_rpc_on_int32_overflow = false;

static const string STR_ERROR("__error", 7);
static const string STR_ERROR_CODE("__error_code", 12);

static const char *last_rpc_error;
static int32_t last_rpc_error_code;

static const int32_t *rpc_data_begin;
static const int32_t *rpc_data;
static int32_t rpc_data_len;
static string rpc_data_copy;
static string rpc_filename;

static const int *rpc_data_begin_backup;
static const int *rpc_data_backup;
static int rpc_data_len_backup;
static string rpc_data_copy_backup;

tl_fetch_wrapper_ptr tl_fetch_wrapper;
array<tl_storer_ptr> tl_storers_ht;

template<class T>
static inline T store_parse_number(const string &v) {
  T result = 0;
  const char *s = v.c_str();
  int sign = 1;
  if (*s == '-') {
    s++;
    sign = -1;
  }
  while ('0' <= *s && *s <= '9') {
    result = result * 10 + (*s++ - '0');
  }
  return result * sign;
}

template<class T>
static inline T store_parse_number(const mixed &v) {
  if (!v.is_string()) {
    if (v.is_float()) {
      return (T)v.to_float();
    }
    return (T)v.to_int();
  }
  return store_parse_number<T>(v.to_string());
}



static void rpc_parse_save_backup() {
  dl::enter_critical_section();//OK
  rpc_data_copy_backup = rpc_data_copy;
  dl::leave_critical_section();

  rpc_data_begin_backup = rpc_data_begin;
  rpc_data_backup = rpc_data;
  rpc_data_len_backup = rpc_data_len;
}

void rpc_parse_restore_previous() {
  php_assert ((rpc_data_copy_backup.size() & 3) == 0);

  dl::enter_critical_section();//OK
  rpc_data_copy = rpc_data_copy_backup;
  rpc_data_copy_backup = tl_str_underscore;//for assert
  dl::leave_critical_section();

  rpc_data_begin = rpc_data_begin_backup;
  rpc_data = rpc_data_backup;
  rpc_data_len = rpc_data_len_backup;
}

const char *last_rpc_error_message_get() {
  return last_rpc_error;
}

int32_t last_rpc_error_code_get() {
  return last_rpc_error_code;
}

void last_rpc_error_reset() {
  last_rpc_error = nullptr;
  last_rpc_error_code = TL_ERROR_UNKNOWN;
}

void rpc_parse(const int32_t *new_rpc_data, int32_t new_rpc_data_len) {
  rpc_parse_save_backup();

  rpc_data_begin = new_rpc_data;
  rpc_data = new_rpc_data;
  rpc_data_len = new_rpc_data_len;
}

bool f$rpc_parse(const string &new_rpc_data) {
  if (new_rpc_data.size() % sizeof(int) != 0) {
    php_warning("Wrong parameter \"new_rpc_data\" of len %d passed to function rpc_parse", (int)new_rpc_data.size());
    last_rpc_error = "Result's length is not divisible by 4";
    last_rpc_error_code = TL_ERROR_RESPONSE_SYNTAX;
    return false;
  }

  rpc_parse_save_backup();

  dl::enter_critical_section();//OK
  rpc_data_copy = new_rpc_data;
  dl::leave_critical_section();

  rpc_data_begin = rpc_data = reinterpret_cast <const int *> (rpc_data_copy.c_str());
  rpc_data_len = static_cast<int>(rpc_data_copy.size() / sizeof(int));
  return true;
}

bool f$rpc_parse(const mixed &new_rpc_data) {
  if (!new_rpc_data.is_string()) {
    php_warning("Parameter 1 of function rpc_parse must be a string, %s is given", new_rpc_data.get_type_c_str());
    return false;
  }

  return f$rpc_parse(new_rpc_data.to_string());
}

bool f$rpc_parse(bool new_rpc_data) {
  return f$rpc_parse(mixed{new_rpc_data});
}

bool f$rpc_parse(const Optional<string> &new_rpc_data) {
  auto rpc_parse_lambda = [](const auto &v) { return f$rpc_parse(v); };
  return call_fun_on_optional_value(rpc_parse_lambda, new_rpc_data);
}

int32_t rpc_get_pos() {
  return static_cast<int32_t>(rpc_data - rpc_data_begin);
}

bool rpc_set_pos(int32_t pos) {
  if (pos < 0 || rpc_data_begin + pos > rpc_data) {
    return false;
  }

  rpc_data_len += static_cast<int32_t>(rpc_data - rpc_data_begin - pos);
  rpc_data = rpc_data_begin + pos;
  return true;
}


static inline void check_rpc_data_len(int64_t len) {
  if (rpc_data_len < len) {
    THROW_EXCEPTION(new_Exception(rpc_filename, __LINE__, string("Not enough data to fetch", 24), -1));
    return;
  }
  rpc_data_len -= static_cast<int32_t>(len);
}

int32_t rpc_lookup_int() {
  TRY_CALL_VOID(int32_t, (check_rpc_data_len(1)));
  rpc_data_len++;
  return *rpc_data;
}

int32_t rpc_fetch_int() {
  TRY_CALL_VOID(int32_t, (check_rpc_data_len(1)));
  return *rpc_data++;
}

int64_t f$fetch_int() {
  return rpc_fetch_int();
}

int64_t f$fetch_lookup_int() {
  return rpc_lookup_int();
}

string f$fetch_lookup_data(int64_t x4_bytes_length) {
  TRY_CALL_VOID(string, (check_rpc_data_len(x4_bytes_length)));
  rpc_data_len += static_cast<int32_t>(x4_bytes_length);
  return {reinterpret_cast<const char *>(rpc_data),
                static_cast<string::size_type>(x4_bytes_length * 4)};
}

int64_t f$fetch_long() {
  TRY_CALL_VOID(int64_t, (check_rpc_data_len(2)));
  long long result = *reinterpret_cast<const long long *>(rpc_data);
  rpc_data += 2;

  return result;
}

double f$fetch_double() {
  TRY_CALL_VOID(double, (check_rpc_data_len(2)));
  double result = *reinterpret_cast<const double *>(rpc_data);
  rpc_data += 2;

  return result;
}

double f$fetch_float() {
  TRY_CALL_VOID(float, (check_rpc_data_len(1)));
  float result = *reinterpret_cast<const float *>(rpc_data);
  rpc_data += 1;

  return result;
}

void f$fetch_raw_vector_double(array<double> &out, int64_t n_elems) {
  int64_t rpc_data_buf_offset = static_cast<int64_t>(sizeof(double) * n_elems / 4);
  TRY_CALL_VOID(void, (check_rpc_data_len(rpc_data_buf_offset)));
  out.memcpy_vector(n_elems, rpc_data);
  rpc_data += rpc_data_buf_offset;
}

static inline const char *f$fetch_string_raw(int *string_len) {
  TRY_CALL_VOID_(check_rpc_data_len(1), return nullptr);
  const char *str = reinterpret_cast <const char *> (rpc_data);
  int result_len = (unsigned char)*str++;
  if (result_len < 254) {
    TRY_CALL_VOID_(check_rpc_data_len(result_len >> 2), return nullptr);
    rpc_data += (result_len >> 2) + 1;
  } else if (result_len == 254) {
    result_len = (unsigned char)str[0] + ((unsigned char)str[1] << 8) + ((unsigned char)str[2] << 16);
    str += 3;
    TRY_CALL_VOID_(check_rpc_data_len((result_len + 3) >> 2), return nullptr);
    rpc_data += ((result_len + 7) >> 2);
  } else {
    THROW_EXCEPTION(new_Exception(rpc_filename, __LINE__, string("Can't fetch string, 255 found", 29), -3));
    return nullptr;
  }

  *string_len = result_len;
  return str;
}

string f$fetch_string() {
  int result_len = 0;
  const char *str = TRY_CALL(const char*, string, f$fetch_string_raw(&result_len));
  return {str, static_cast<string::size_type>(result_len)};
}

int64_t f$fetch_string_as_int() {
  int result_len = 0;
  const char *str = TRY_CALL(const char*, int, f$fetch_string_raw(&result_len));
  return string::to_int(str, static_cast<string::size_type>(result_len));
}

mixed f$fetch_memcache_value() {
  static constexpr int32_t MEMCACHE_VALUE_NOT_FOUND = 0x32c42422;
  static constexpr int32_t MEMCACHE_VALUE_LONG = 0x9729c42;
  static constexpr int32_t MEMCACHE_VALUE_STRING = 0xa6bebb1a;

  int res = TRY_CALL(int, bool, rpc_fetch_int());
  switch (res) {
    case MEMCACHE_VALUE_STRING: {
      int value_len = 0;
      const char *value = TRY_CALL(const char*, bool, f$fetch_string_raw(&value_len));
      int flags = TRY_CALL(int, bool, rpc_fetch_int());
      return mc_get_value(value, value_len, flags);
    }
    case MEMCACHE_VALUE_LONG: {
      mixed value = TRY_CALL(mixed, bool, f$fetch_long());
      int flags = TRY_CALL(int, bool, rpc_fetch_int());

      if (flags != 0) {
        php_warning("Wrong parameter flags = %d returned in Memcache::get", flags);
      }

      return value;
    }
    case MEMCACHE_VALUE_NOT_FOUND: {
      return false;
    }
    default: {
      php_warning("Wrong memcache.Value constructor = %x", res);
      THROW_EXCEPTION(new_Exception(rpc_filename, __LINE__, string("Wrong memcache.Value constructor"), -1));
      return {};
    }
  }
}

bool f$fetch_eof() {
  return rpc_data_len == 0;
}

bool f$fetch_end() {
  if (rpc_data_len) {
    THROW_EXCEPTION(new_Exception(rpc_filename, __LINE__, string("Too much data to fetch"), -2));
    return false;
  }
  return true;
}

C$RpcConnection::C$RpcConnection(int32_t host_num, int32_t port, int32_t timeout_ms, int64_t actor_id, int32_t connect_timeout, int32_t reconnect_timeout) :
  host_num(host_num),
  port(port),
  timeout_ms(timeout_ms),
  actor_id(actor_id),
  connect_timeout(connect_timeout),
  reconnect_timeout(reconnect_timeout) {
}

class_instance<C$RpcConnection> f$new_rpc_connection(const string &host_name, int64_t port, const mixed &actor_id, double timeout, double connect_timeout, double reconnect_timeout) {
  int32_t host_num = rpc_connect_to(host_name.c_str(), static_cast<int32_t>(port));
  if (host_num < 0) {
    return {};
  }

  return make_instance<C$RpcConnection>(host_num, static_cast<int32_t>(port), timeout_convert_to_ms(timeout),
                                        store_parse_number<int64_t >(actor_id),
                                        timeout_convert_to_ms(connect_timeout), timeout_convert_to_ms(reconnect_timeout));
}

static string_buffer data_buf;

bool rpc_stored;
static int64_t rpc_pack_threshold;
static int64_t rpc_pack_from;

void estimate_and_flush_overflow(size_t &bytes_sent) {
  // estimate
  bytes_sent += data_buf.size();
  if (bytes_sent >= (1 << 15) && bytes_sent > data_buf.size()) {
    f$rpc_flush();
    bytes_sent = data_buf.size();
  }
}

void f$store_gzip_pack_threshold(int64_t pack_threshold_bytes) {
  rpc_pack_threshold = pack_threshold_bytes;
}

void f$store_start_gzip_pack() {
  rpc_pack_from = data_buf.size();
}

void f$store_finish_gzip_pack(int64_t threshold) {
  if (rpc_pack_from != -1 && threshold > 0) {
    int64_t answer_size = data_buf.size() - rpc_pack_from;
    php_assert (rpc_pack_from % sizeof(int) == 0 && 0 <= rpc_pack_from && 0 <= answer_size);
    if (answer_size >= threshold) {
      const char *answer_begin = data_buf.c_str() + rpc_pack_from;
      const string_buffer *compressed = zlib_encode(answer_begin, static_cast<int32_t>(answer_size), 6, ZLIB_ENCODE);

      if (compressed->size() + 2 * sizeof(int) < answer_size) {
        data_buf.set_pos(rpc_pack_from);
        store_int(GZIP_PACKED);
        store_string(compressed->buffer(), compressed->size());
      }
    }
  }
  rpc_pack_from = -1;
}


template<class T>
inline bool store_raw(T v) {
  data_buf.append(reinterpret_cast<char *>(&v), sizeof(v));
  return true;
}

bool f$store_raw(const string &data) {
  size_t data_len = data.size();
  if (data_len & 3) {
    return false;
  }
  data_buf.append(data.c_str(), data_len);
  return true;
}

void f$store_raw_vector_double(const array<double> &vector) {
  data_buf.append(reinterpret_cast<const char *>(vector.get_const_vector_pointer()),
                  sizeof(double) * vector.count());
}

bool store_header(long long cluster_id, int64_t flags) {
  if (flags) {
    store_int(TL_RPC_DEST_ACTOR_FLAGS);
    store_long(cluster_id);
    f$store_int(flags);
  } else {
    store_int(TL_RPC_DEST_ACTOR);
    store_long(cluster_id);
  }
  return true;
}

bool f$store_header(const mixed &cluster_id, int64_t flags) {
  return store_header(store_parse_number<long long>(cluster_id), flags);
}

bool store_error(int64_t error_code, const char *error_text, int error_text_len) {
  f$rpc_clean(true);
  f$store_int(error_code);
  store_string(error_text, error_text_len);
  rpc_store(true);
  script_error();
  return true;
}

bool f$store_error(int64_t error_code, const string &error_text) {
  return store_error(error_code, error_text.c_str(), (int)error_text.size());
}

bool f$set_fail_rpc_on_int32_overflow(bool fail_rpc) {
  fail_rpc_on_int32_overflow = fail_rpc;
  return true;
}

bool is_int32_overflow(int64_t v) {
  // f$store_int function is used for int and 'magic' storing,
  // 'magic' can he assigned via hex literals which may set the 32nd bit,
  // this is why we additionally check for the uint32_t here
  const auto v32 = static_cast<int32_t>(v);
  return vk::none_of_equal(v, int64_t{v32}, int64_t{static_cast<uint32_t>(v32)});
}

bool store_int(int32_t v) {
  return store_raw(v);
}

bool f$store_int(int64_t v) {
  const auto v32 = static_cast<int32_t>(v);
  if (unlikely(is_int32_overflow(v))) {
    php_warning("Got int32 overflow on storing '%" PRIi64 "', the value will be casted to '%d'", v, v32);
  }
  return store_int(v32);
}

bool store_long(long long v) {
  return store_raw(v);
}

bool f$store_double(double v) {
  return store_raw(v);
}

bool f$store_float(double v) {
  return store_raw(static_cast<float>(v));
}

bool store_string(const char *v, int32_t v_len) {
  int32_t all_len = v_len;
  if (v_len < 254) {
    data_buf << (char)(v_len);
    all_len += 1;
  } else if (v_len < (1 << 24)) {
    data_buf
      << (char)(254)
      << (char)(v_len & 255)
      << (char)((v_len >> 8) & 255)
      << (char)((v_len >> 16) & 255);
    all_len += 4;
  } else {
    php_critical_error ("trying to store too big string of length %d", v_len);
  }
  data_buf.append(v, static_cast<size_t>(v_len));

  while (all_len % 4 != 0) {
    data_buf << '\0';
    all_len++;
  }
  return true;
}

bool f$store_string(const string &v) {
  return store_string(v.c_str(), (int)v.size());
}

bool f$store_many(const array<mixed> &a) {
  int64_t n = a.count();
  if (n == 0) {
    php_warning("store_many must take at least 1 argument");
    return false;
  }

  string pattern = a.get_value(0).to_string();
  if (n != 1 + pattern.size()) {
    php_warning("Wrong number of arguments in call to store_many");
    return false;
  }

  for (int64_t i = 1; i < n; i++) {
    switch (pattern[static_cast<string::size_type>(i - 1)]) {
      case 's':
        f$store_string(a.get_value(i).to_string());
        break;
      case 'l':
        f$store_long(a.get_value(i).to_int());
        break;
      case 'd':
      case 'i':
        f$store_int(a.get_value(i).to_int());
        break;
      case 'f':
        f$store_double(a.get_value(i).to_float());
        break;
      default:
        php_warning("Wrong symbol '%c' at position %" PRIi64 " in first argument of store_many", pattern[static_cast<string::size_type>(i - 1)], i - 1);
        break;
    }
  }

  return true;
}


bool f$store_finish() {
  return rpc_store(false);
}

bool f$rpc_clean(bool is_error) {
  data_buf.clean();

  store_raw(RpcHeaders{-is_error});

  rpc_pack_from = -1;
  return true;
}

bool rpc_store(bool is_error) {
  if (rpc_stored) {
    return false;
  }

  if (!is_error) {
    rpc_pack_from = sizeof(RpcHeaders);
    f$store_finish_gzip_pack(rpc_pack_threshold);
  }

  store_int(-1); // reserve for crc32
  rpc_stored = true;
  rpc_answer(data_buf.c_str(), static_cast<int>(data_buf.size()));
  return true;
}


// only for good linkage. Will be never used to load
template<>
int Storage::tagger<rpc_request>::get_tag() noexcept {
  return 1960913044;
}

static rpc_request *rpc_requests;
static int rpc_requests_size;
static long long rpc_requests_last_query_num;

static slot_id_t rpc_first_request_id;
static slot_id_t rpc_first_array_request_id;
static slot_id_t rpc_next_request_id;
static slot_id_t rpc_first_unfinished_request_id;

static rpc_request gotten_rpc_request;

static int timeout_wakeup_id = -1;

rpc_request *get_rpc_request(slot_id_t request_id) {
  php_assert (rpc_first_request_id <= request_id && request_id < rpc_next_request_id);
  if (request_id < rpc_first_array_request_id) {
    return &gotten_rpc_request;
  }
  return &rpc_requests[request_id - rpc_first_array_request_id];
}

class rpc_resumable : public Resumable {
private:
  int request_id;

protected:
  bool run() {
    php_assert (dl::query_num == rpc_requests_last_query_num);
    rpc_request *request = get_rpc_request(request_id);
    php_assert (request->resumable_id < 0);
    php_assert (input_ == nullptr);

/*
    if (request->resumable_id == -1) {
      int len = *reinterpret_cast <int *>(request->answer - 12);
      fprintf (stderr, "Receive  string of len %d at %p\n", len, request->answer);
      for (int i = -12; i <= len; i++) {
        fprintf (stderr, "%d: %x(%d)\t%c\n", i, request->answer[i], request->answer[i], request->answer[i] >= 32 ? request->answer[i] : '.');
      }
    }
*/
    if (rpc_first_unfinished_request_id == request_id) {
      while (rpc_first_unfinished_request_id < rpc_next_request_id &&
             get_rpc_request(rpc_first_unfinished_request_id)->resumable_id < 0) {
        rpc_first_unfinished_request_id++;
      }
      if (rpc_first_unfinished_request_id < rpc_next_request_id) {
        int64_t resumable_id = get_rpc_request(rpc_first_unfinished_request_id)->resumable_id;
        php_assert (resumable_id > 0);
        const Resumable *resumable = get_forked_resumable(resumable_id);
        php_assert (resumable != nullptr);
      }
    }

    request_id = -1;
    output_->save<rpc_request>(*request);
    php_assert (request->resumable_id == -2 || request->resumable_id == -1);
    request->resumable_id = -3;
    request->answer = nullptr;

    return true;
  }

public:
  explicit rpc_resumable(int request_id) :
    request_id(request_id) {
  }
};

static array<double> rpc_request_need_timer;

static void process_rpc_timeout(int request_id) {
  process_rpc_error(request_id, TL_ERROR_QUERY_TIMEOUT, "Timeout in KPHP runtime");
}

static void process_rpc_timeout(kphp_event_timer *timer) {
  return process_rpc_timeout(timer->wakeup_extra);
}

int64_t rpc_send(const class_instance<C$RpcConnection> &conn, double timeout, bool ignore_answer) {
  if (unlikely (conn.is_null() || conn.get()->host_num < 0)) {
    php_warning("Wrong RpcConnection specified");
    return -1;
  }

  if (timeout <= 0 || timeout > MAX_TIMEOUT) {
    timeout = conn.get()->timeout_ms * 0.001;
  }

  store_int(-1); // reserve for crc32
  php_assert (data_buf.size() % sizeof(int) == 0);

  uint32_t function_magic = *reinterpret_cast<const uint32_t *>(data_buf.c_str() + sizeof(RpcHeaders));
  RpcExtraHeaders extra_headers{};
  size_t extra_headers_size = fill_extra_headers_if_needed(extra_headers, function_magic, conn.get()->actor_id, ignore_answer);

  const auto request_size = static_cast<size_t>(data_buf.size() + extra_headers_size);
  char *p = static_cast<char *>(dl::allocate(request_size));

  // Memory will look like this:
  //    [ RpcHeaders (reserved in f$rpc_clean) ] [ RpcExtraHeaders (optional) ] [ payload ]
  memcpy(p, data_buf.c_str(), sizeof(RpcHeaders));
  memcpy(p + sizeof(RpcHeaders), &extra_headers, extra_headers_size);
  memcpy(p + sizeof(RpcHeaders) + extra_headers_size, data_buf.c_str() + sizeof(RpcHeaders), data_buf.size() - sizeof(RpcHeaders));

  slot_id_t result = rpc_send_query(conn.get()->host_num, p, static_cast<int>(request_size), timeout_convert_to_ms(timeout));
  if (result <= 0) {
    return -1;
  }

  if (dl::query_num != rpc_requests_last_query_num) {
    rpc_requests_last_query_num = dl::query_num;
    rpc_requests_size = 170;
    rpc_requests = static_cast<rpc_request *>(dl::allocate(sizeof(rpc_request) * rpc_requests_size));

    rpc_first_request_id = result;
    rpc_first_array_request_id = result;
    rpc_next_request_id = result + 1;
    rpc_first_unfinished_request_id = result;
    gotten_rpc_request.resumable_id = -3;
    gotten_rpc_request.answer = nullptr;
  } else {
    php_assert (rpc_next_request_id == result);
    rpc_next_request_id++;
  }

  if (result - rpc_first_array_request_id >= rpc_requests_size) {
    php_assert (result - rpc_first_array_request_id == rpc_requests_size);
    if (rpc_first_unfinished_request_id > rpc_first_array_request_id + rpc_requests_size / 2) {
      memcpy(rpc_requests,
             rpc_requests + rpc_first_unfinished_request_id - rpc_first_array_request_id,
             sizeof(rpc_request) * (rpc_requests_size - (rpc_first_unfinished_request_id - rpc_first_array_request_id)));
      rpc_first_array_request_id = rpc_first_unfinished_request_id;
    } else {
      rpc_requests = static_cast <rpc_request *> (dl::reallocate(rpc_requests, sizeof(rpc_request) * 2 * rpc_requests_size, sizeof(rpc_request) * rpc_requests_size));
      rpc_requests_size *= 2;
    }
  }

  rpc_request *cur = get_rpc_request(result);

  cur->resumable_id = register_forked_resumable(new rpc_resumable(result));
  cur->function_magic = function_magic;
  cur->actor_id = conn.get()->actor_id;
  cur->timer = nullptr;
  if (ignore_answer) {
    int64_t resumable_id = cur->resumable_id;
    process_rpc_timeout(result);
    get_forked_storage(resumable_id)->load<rpc_request>();
    return resumable_id;
  } else {
    rpc_request_need_timer.set_value(result, timeout);
    return cur->resumable_id;
  }
}

void f$rpc_flush() {
  update_precise_now();
  wait_net(0);
  update_precise_now();
  for (array<double>::iterator iter = rpc_request_need_timer.begin(); iter != rpc_request_need_timer.end(); ++iter) {
    int32_t id = static_cast<int32_t>(iter.get_key().to_int());
    rpc_request *cur = get_rpc_request(id);
    if (cur->resumable_id > 0) {
      php_assert (cur->timer == nullptr);
      cur->timer = allocate_event_timer(iter.get_value() + get_precise_now(), timeout_wakeup_id, id);
    }
  }
  rpc_request_need_timer.clear();
}

int64_t f$rpc_send(const class_instance<C$RpcConnection> &conn, double timeout) {
  int64_t request_id = rpc_send(conn, timeout);
  if (request_id <= 0) {
    return 0;
  }

  f$rpc_flush();
  return request_id;
}

int64_t f$rpc_send_noflush(const class_instance<C$RpcConnection> &conn, double timeout) {
  int64_t request_id = rpc_send(conn, timeout);
  if (request_id <= 0) {
    return 0;
  }

  return request_id;
}

void process_rpc_answer(int32_t request_id, char *result, int32_t result_len __attribute__((unused))) {
  rpc_request *request = get_rpc_request(request_id);

  if (request->resumable_id < 0) {
    php_assert (result != nullptr);
    dl::deallocate(result - 12, result_len + 13);
    php_assert (request->resumable_id != -1);
    return;
  }
  int64_t resumable_id = request->resumable_id;
  request->resumable_id = -1;

  if (request->timer) {
    remove_event_timer(request->timer);
  }

  php_assert (result != nullptr);
  request->answer = result;
//  fprintf (stderr, "answer_len = %d\n", result_len);

  php_assert (resumable_id > 0);
  resumable_run_ready(resumable_id);
}

void process_rpc_error(int32_t request_id, int32_t error_code, const char *error_message) {
  rpc_request *request = get_rpc_request(request_id);

  if (request->resumable_id < 0) {
    php_assert (request->resumable_id != -1);
    return;
  }
  int64_t resumable_id = request->resumable_id;
  request->resumable_id = -2;

  if (request->timer) {
    remove_event_timer(request->timer);
  }

  request->error = error_message;
  request->error_code = error_code;

  php_assert (resumable_id > 0);
  resumable_run_ready(resumable_id);
}


class rpc_get_resumable : public Resumable {
  using ReturnT = Optional<string>;
  int64_t resumable_id;
  double timeout;

  bool ready;
protected:
  bool run() {
    RESUMABLE_BEGIN
      ready = wait_without_result(resumable_id, timeout);
      TRY_WAIT(rpc_get_resumable_label_0, ready, bool);
      if (!ready) {
        last_rpc_error = last_wait_error;
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }

      Storage *input = get_forked_storage(resumable_id);
      if (input->tag == 0) {
        last_rpc_error = "Result already was gotten";
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }
      if (input->tag != Storage::tagger<rpc_request>::get_tag()) {
        last_rpc_error = "Not a rpc request";
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }

      rpc_request res = input->load<rpc_request>();
      php_assert (CurException.is_null());

      if (res.resumable_id == -2) {
        last_rpc_error = res.error;
        last_rpc_error_code = res.error_code;
        RETURN(false);
      }

      php_assert (res.resumable_id == -1);

      string result;
      result.assign_raw(res.answer - 12);
      RETURN(result);
    RESUMABLE_END
  }

public:
  rpc_get_resumable(int64_t resumable_id, double timeout) :
    resumable_id(resumable_id),
    timeout(timeout),
    ready(false) {
  }
};

bool drop_tl_query_info(int64_t query_id) {
  auto query = RpcPendingQueries::get().withdraw(query_id);
  if (query.is_null()) {
    php_warning("Result of TL query with id %" PRIi64 " has already been taken or id is incorrect", query_id);
    return false;
  }
  return true;
}

Optional<string> f$rpc_get(int64_t request_id, double timeout) {
  if (!drop_tl_query_info(request_id)) {
    return false;
  }
  return start_resumable<Optional<string>>(new rpc_get_resumable(request_id, timeout));
}

Optional<string> f$rpc_get_synchronously(int64_t request_id) {
  wait_without_result_synchronously(request_id);
  Optional<string> result = f$rpc_get(request_id);
  php_assert (resumable_finished);
  return result;
}

class rpc_get_and_parse_resumable : public Resumable {
  using ReturnT = bool;
  int64_t resumable_id;
  double timeout;

  bool ready;
protected:
  bool run() {
    RESUMABLE_BEGIN
      ready = wait_without_result(resumable_id, timeout);
      TRY_WAIT(rpc_get_and_parse_resumable_label_0, ready, bool);
      if (!ready) {
        last_rpc_error = last_wait_error;
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }

      Storage *input = get_forked_storage(resumable_id);
      if (input->tag == 0) {
        last_rpc_error = "Result already was gotten";
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }
      if (input->tag != Storage::tagger<rpc_request>::get_tag()) {
        last_rpc_error = "Not a rpc request";
        last_rpc_error_code = TL_ERROR_INTERNAL;
        RETURN(false);
      }

      rpc_request res = input->load<rpc_request>();
      php_assert (CurException.is_null());

      if (res.resumable_id == -2) {
        last_rpc_error = res.error;
        last_rpc_error_code = res.error_code;
        RETURN(false);
      }

      php_assert (res.resumable_id == -1);

      string result;
      result.assign_raw(res.answer - 12);
      bool parse_result = f$rpc_parse(result);
      php_assert(parse_result);

      RETURN(true);
    RESUMABLE_END
  }

public:
  rpc_get_and_parse_resumable(int64_t resumable_id, double timeout) :
    resumable_id(resumable_id),
    timeout(timeout),
    ready(false) {
  }
};

bool f$rpc_get_and_parse(int64_t request_id, double timeout) {
  if (!drop_tl_query_info(request_id)) {
    return false;
  }
  return rpc_get_and_parse(request_id, timeout);
}

bool rpc_get_and_parse(int64_t request_id, double timeout) {
  return start_resumable<bool>(new rpc_get_and_parse_resumable(request_id, timeout));
}


int64_t f$query_x2(int64_t x) {
  return query_x2(static_cast<int32_t>(x));
}


/*
 *
 *  mixed wrappers
 *
 */


bool store_long(const mixed &v) {
  return store_long(store_parse_number<long long>(v));
}

bool f$store_long(int64_t v) {
  return store_long(v);
}


/*
 *
 *     RPC_TL_QUERY
 *
 */


int32_t tl_parse_int() {
  return TRY_CALL(int32_t, int32_t, (rpc_fetch_int()));
}

long long tl_parse_long() {
  TRY_CALL_VOID(int, check_rpc_data_len(2));
  long long result = *reinterpret_cast<const long long *>(rpc_data);
  rpc_data += 2;

  return result;
}

double tl_parse_double() {
  return TRY_CALL(double, double, (f$fetch_double()));
}

double tl_parse_float() {
  return TRY_CALL(double, double, (f$fetch_float()));
}

string tl_parse_string() {
  return TRY_CALL(string, string, (f$fetch_string()));
}

void tl_parse_end() {
  TRY_CALL_VOID(void, (f$fetch_end()));
}

int tl_parse_save_pos() {
  return rpc_get_pos();
}

bool tl_parse_restore_pos(int pos) {
  return rpc_set_pos(pos);
}

array<mixed> tl_fetch_error(const string &error, int error_code) {
  array<mixed> result;
  result.set_value(STR_ERROR, error);
  result.set_value(STR_ERROR_CODE, error_code);
  return result;
}

array<mixed> tl_fetch_error(const char *error, int error_code) {
  return tl_fetch_error(string(error), error_code);
}

long long rpc_tl_results_last_query_num = -1;

bool try_fetch_rpc_error(array<mixed> &out_if_error) {
  int x = rpc_lookup_int();
  if (x == TL_RPC_REQ_ERROR && CurException.is_null()) {
    php_assert (tl_parse_int() == TL_RPC_REQ_ERROR);
    if (CurException.is_null()) {
      tl_parse_long();
      if (CurException.is_null()) {
        int error_code = tl_parse_int();
        if (CurException.is_null()) {
          string error = tl_parse_string();
          if (CurException.is_null()) {
            out_if_error = tl_fetch_error(error, error_code);
            return true;
          }
        }
      }
    }
  }
  if (!CurException.is_null()) {
    out_if_error = tl_fetch_error(CurException->$message, TL_ERROR_SYNTAX);
    CurException = Optional<bool>{};
    return true;
  }
  return false;
}

class_instance<RpcTlQuery> store_function(const mixed &tl_object) {
  php_assert(CurException.is_null());
  if (!tl_object.is_array()) {
    CurrentProcessingQuery::get().raise_storing_error("Not an array passed to function rpc_tl_query");
    return {};
  }
  string fun_name = tl_arr_get(tl_object, tl_str_underscore, 0).to_string();
  if (!tl_storers_ht.has_key(fun_name)) {
    CurrentProcessingQuery::get().raise_storing_error("Function \"%s\" not found in tl-scheme", fun_name.c_str());
    return {};
  }
  class_instance<RpcTlQuery> rpc_query;
  rpc_query.alloc();
  rpc_query.get()->tl_function_name = fun_name;
  CurrentProcessingQuery::get().set_current_tl_function(fun_name);
  const auto &untyped_storer = tl_storers_ht.get_value(fun_name);
  rpc_query.get()->result_fetcher = make_unique_on_script_memory<RpcRequestResultUntyped>(untyped_storer(tl_object));
  CurrentProcessingQuery::get().reset();
  return rpc_query;
}

array<mixed> fetch_function(const class_instance<RpcTlQuery> &rpc_query) {
  array<mixed> new_tl_object;
  if (try_fetch_rpc_error(new_tl_object)) {
    return new_tl_object;       // this object carries an error (see tl_fetch_error())
  }
  php_assert(!rpc_query.is_null());
  CurrentProcessingQuery::get().set_current_tl_function(rpc_query);
  auto stored_fetcher = rpc_query.get()->result_fetcher->extract_untyped_fetcher();
  php_assert(stored_fetcher);
  new_tl_object = tl_fetch_wrapper(std::move(stored_fetcher));
  CurrentProcessingQuery::get().reset();
  if (!CurException.is_null()) {
    array<mixed> result = tl_fetch_error(CurException->$message, TL_ERROR_SYNTAX);
    CurException = Optional<bool>{};
    return result;
  }
  if (!f$fetch_eof()) {
    php_warning("Not all data fetched");
    return tl_fetch_error("Not all data fetched", TL_ERROR_EXTRA_DATA);
  }
  return new_tl_object;
}

int64_t rpc_tl_query_impl(const class_instance<C$RpcConnection> &c, const mixed &tl_object, double timeout, bool ignore_answer, bool bytes_estimating, size_t &bytes_sent, bool flush) {
  f$rpc_clean();

  class_instance<RpcTlQuery> rpc_query = store_function(tl_object);
  if (!CurException.is_null()) {
    rpc_query.destroy();
    CurException = Optional<bool>{};
  }
  if (rpc_query.is_null()) {
    return 0;
  }

  if (bytes_estimating) {
    estimate_and_flush_overflow(bytes_sent);
  }
  int64_t query_id = rpc_send(c, timeout, ignore_answer);
  if (query_id <= 0) {
    return 0;
  }
  if (flush) {
    f$rpc_flush();
  }
  if (ignore_answer) {
    return -1;
  }
  if (dl::query_num != rpc_tl_results_last_query_num) {
    rpc_tl_results_last_query_num = dl::query_num;
  }
  rpc_query.get()->query_id = query_id;
  RpcPendingQueries::get().save(rpc_query);
  
  return query_id;
}

int64_t f$rpc_tl_query_one(const class_instance<C$RpcConnection> &c, const mixed &tl_object, double timeout) {
  size_t bytes_sent = 0;
  return rpc_tl_query_impl(c, tl_object, timeout, false, false, bytes_sent, true);
}

int64_t f$rpc_tl_pending_queries_count() {
  if (dl::query_num != rpc_tl_results_last_query_num) {
    return 0;
  }
  return RpcPendingQueries::get().count();
}

bool f$rpc_mc_parse_raw_wildcard_with_flags_to_array(const string &raw_result, array<mixed> &result) {
  if (raw_result.empty() || !f$rpc_parse(raw_result)) {
    return false;
  };

  int magic = TRY_CALL_ (int, rpc_fetch_int(), return false);
  if (magic != TL_DICTIONARY) {
    THROW_EXCEPTION(new_Exception(rpc_filename, __LINE__, string("Strange dictionary magic", 24), -1));
    return false;
  };

  int cnt = TRY_CALL_ (int, rpc_fetch_int(), return false);
  if (cnt == 0) {
    return true;
  };
  result.reserve(0, cnt + f$count(result), false);

  for (int j = 0; j < cnt; ++j) {
    string key = f$fetch_string();

    if (!CurException.is_null()) {
      return false;
    }

    mixed value = f$fetch_memcache_value();

    if (!CurException.is_null()) {
      return false;
    }

    result.set_value(key, value);
  };

  return true;
}

array<int64_t> f$rpc_tl_query(const class_instance<C$RpcConnection> &c, const array<mixed> &tl_objects, double timeout, bool ignore_answer) {
  array<int64_t> result(tl_objects.size());
  size_t bytes_sent = 0;
  for (auto it = tl_objects.begin(); it != tl_objects.end(); ++it) {
    int64_t query_id = rpc_tl_query_impl(c, it.get_value(), timeout, ignore_answer, true, bytes_sent, false);
    result.set_value(it.get_key(), query_id);
  }
  if (bytes_sent > 0) {
    f$rpc_flush();
  }

  return result;
}


class rpc_tl_query_result_one_resumable : public Resumable {
  using ReturnT = array<mixed>;

  int64_t query_id;
  class_instance<RpcTlQuery> rpc_query;
protected:
  bool run() {
    bool ready;

    RESUMABLE_BEGIN
      last_rpc_error_reset();
      ready = rpc_get_and_parse(query_id, -1);
      TRY_WAIT(rpc_get_and_parse_resumable_label_0, ready, bool);
      if (!ready) {
        php_assert (last_rpc_error != nullptr);
        if (!rpc_query.is_null()) {
          rpc_query.get()->result_fetcher.reset();
        }
        RETURN(tl_fetch_error(last_rpc_error, last_rpc_error_code));
      }

      array<mixed> tl_object = fetch_function(rpc_query);
      rpc_parse_restore_previous();
      RETURN(tl_object);
    RESUMABLE_END
  }

public:
  rpc_tl_query_result_one_resumable(int64_t query_id, class_instance<RpcTlQuery> &&rpc_query) :
    query_id(query_id),
    rpc_query(std::move(rpc_query)) {
  }
};


array<mixed> f$rpc_tl_query_result_one(int64_t query_id) {
  auto resumable_finalizer = vk::finally([] { resumable_finished = true; });

  if (query_id <= 0) {
    return tl_fetch_error("Wrong query_id", TL_ERROR_WRONG_QUERY_ID);
  }

  if (dl::query_num != rpc_tl_results_last_query_num) {
    return tl_fetch_error("There were no TL queries in current script run", TL_ERROR_INTERNAL);
  }

  class_instance<RpcTlQuery> rpc_query = RpcPendingQueries::get().withdraw(query_id);
  if (rpc_query.is_null()) {
    return tl_fetch_error("Can't use rpc_tl_query_result for non-TL query", TL_ERROR_INTERNAL);
  }

  if (!rpc_query.get()->result_fetcher || rpc_query.get()->result_fetcher->empty()) {
    return tl_fetch_error("Rpc query has empty result fetcher", TL_ERROR_INTERNAL);
  }

  if (rpc_query.get()->result_fetcher->is_typed) {
    return tl_fetch_error("Can't get untyped result from typed TL query. Use consistent API for that", TL_ERROR_INTERNAL);
  }

  resumable_finalizer.disable();
  return start_resumable<array<mixed>>(new rpc_tl_query_result_one_resumable(query_id, std::move(rpc_query)));
}


class rpc_tl_query_result_resumable : public Resumable {
  using ReturnT = array<array<mixed>>;

  const array<int64_t> query_ids;
  array<array<mixed>> tl_objects_unsorted;
  int64_t queue_id;
  Optional<int64_t> query_id;

protected:
  bool run() {
    RESUMABLE_BEGIN
      if (query_ids.count() == 1) {
        query_id = query_ids.begin().get_value();

        tl_objects_unsorted[query_id] = f$rpc_tl_query_result_one(query_id.val());
        TRY_WAIT(rpc_tl_query_result_resumable_label_0, tl_objects_unsorted[query_id], array<mixed>);
      } else {
        queue_id = wait_queue_create(query_ids);

        while (true) {
          query_id = f$wait_queue_next(queue_id, -1);
          TRY_WAIT(rpc_tl_query_result_resumable_label_1, query_id, decltype(query_id));
          if (query_id.val() <= 0) {
            break;
          }
          tl_objects_unsorted[query_id] = f$rpc_tl_query_result_one(query_id.val());
          php_assert (resumable_finished);
        }

        unregister_wait_queue(queue_id);
      }

      array<array<mixed>> tl_objects(query_ids.size());
      for (auto it = query_ids.begin(); it != query_ids.end(); ++it) {
        int64_t query_id = it.get_value();
        if (!tl_objects_unsorted.isset(query_id)) {
          if (query_id <= 0) {
            tl_objects[it.get_key()] = tl_fetch_error((static_SB.clean() << "Very wrong query_id " << query_id).str(), TL_ERROR_WRONG_QUERY_ID);
          } else {
            tl_objects[it.get_key()] = tl_fetch_error((static_SB.clean() << "No answer received or duplicate/wrong query_id "
                                                                         << query_id).str(), TL_ERROR_WRONG_QUERY_ID);
          }
        } else {
          tl_objects[it.get_key()] = tl_objects_unsorted[query_id];
        }
      }

      RETURN(tl_objects);
    RESUMABLE_END
  }

public:
  explicit rpc_tl_query_result_resumable(const array<int64_t> &query_ids) :
    query_ids(query_ids),
    tl_objects_unsorted(array_size(query_ids.count(), 0, false)),
    queue_id(0),
    query_id(0) {
  }
};

array<array<mixed>> f$rpc_tl_query_result(const array<int64_t> &query_ids) {
  return start_resumable<array<array<mixed>>>(new rpc_tl_query_result_resumable(query_ids));
}

array<array<mixed>> f$rpc_tl_query_result_synchronously(const array<int64_t> &query_ids) {
  array<array<mixed>> tl_objects_unsorted(array_size(query_ids.count(), 0, false));
  if (query_ids.count() == 1) {
    wait_without_result_synchronously_safe(query_ids.begin().get_value());
    tl_objects_unsorted[query_ids.begin().get_value()] = f$rpc_tl_query_result_one(query_ids.begin().get_value());
    php_assert (resumable_finished);
  } else {
    int64_t queue_id = wait_queue_create(query_ids);

    while (true) {
      int64_t query_id = f$wait_queue_next_synchronously(queue_id).val();
      if (query_id <= 0) {
        break;
      }
      tl_objects_unsorted[query_id] = f$rpc_tl_query_result_one(query_id);
      php_assert (resumable_finished);
    }

    unregister_wait_queue(queue_id);
  }

  array<array<mixed>> tl_objects(query_ids.size());
  for (auto it = query_ids.begin(); it != query_ids.end(); ++it) {
    int64_t query_id = it.get_value();
    if (!tl_objects_unsorted.isset(query_id)) {
      if (query_id <= 0) {
        tl_objects[it.get_key()] = tl_fetch_error((static_SB.clean() << "Very wrong query_id " << query_id).str(), TL_ERROR_WRONG_QUERY_ID);
      } else {
        tl_objects[it.get_key()] = tl_fetch_error((static_SB.clean() << "No answer received or duplicate/wrong query_id "
                                                                     << query_id).str(), TL_ERROR_WRONG_QUERY_ID);
      }
    } else {
      tl_objects[it.get_key()] = tl_objects_unsorted[query_id];
    }
  }

  return tl_objects;
}

void global_init_rpc_lib() {
  php_assert (timeout_wakeup_id == -1);

  timeout_wakeup_id = register_wakeup_callback(&process_rpc_timeout);
}

static void reset_rpc_global_vars() {
  hard_reset_var(rpc_filename);
  hard_reset_var(rpc_data_copy);
  hard_reset_var(rpc_data_copy_backup);
  hard_reset_var(rpc_request_need_timer);
  fail_rpc_on_int32_overflow = false;
}

void init_rpc_lib() {
  php_assert (timeout_wakeup_id != -1);

  CurrentProcessingQuery::get().reset();
  RpcPendingQueries::get().hard_reset();
  CurrentRpcServerQuery::get().reset();
  reset_rpc_global_vars();

  rpc_parse(nullptr, 0);
  // init backup
  rpc_parse(nullptr, 0);

  f$rpc_clean(false);
  rpc_stored = false;

  rpc_pack_threshold = -1;
  rpc_pack_from = -1;
  rpc_filename = string("rpc.cpp", 7);
}

void free_rpc_lib() {
  reset_rpc_global_vars();
  RpcPendingQueries::get().hard_reset();
  CurrentProcessingQuery::get().reset();
}

int64_t f$rpc_queue_create() {
  return f$wait_queue_create();
}

int64_t f$rpc_queue_create(const mixed &request_ids) {
  return f$wait_queue_create(request_ids);
}

int64_t f$rpc_queue_push(int64_t queue_id, const mixed &request_ids) {
  return f$wait_queue_push(queue_id, request_ids);
}

bool f$rpc_queue_empty(int64_t queue_id) {
  return f$wait_queue_empty(queue_id);
}

Optional<int64_t> f$rpc_queue_next(int64_t queue_id, double timeout) {
  return f$wait_queue_next(queue_id, timeout);
}

Optional<int64_t> f$rpc_queue_next_synchronously(int64_t queue_id) {
  return f$wait_queue_next_synchronously(queue_id);
}

bool f$rpc_wait(int64_t request_id) {
  return wait_without_result(request_id);
}

bool f$rpc_wait_concurrently(int64_t request_id) {
  return f$wait_concurrently(request_id);
}
