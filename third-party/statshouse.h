/* Copyright 2022 V Kontakte LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

// Header-only no dependency c++11 compatible implementation of statshouse UDP transport
// tested on linux for now. Implementations for incompatible platforms should use simple #ifdefs around network code
// packing of integers is platform-independent, packing of double may need #ifdef in doubleBits function

// should compile without warnings with -Wno-noexcept-type -g -Wall -Wextra -Werror=return-type

#define STATSHOUSE_TRANSPORT_VERSION "2023-06-14"
#define STATSHOUSE_USAGE_METRICS "statshouse_transport_metrics"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Use #ifdefs to include headers for various platforms here
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define STATSHOUSE_UNLIKELY(x) __builtin_expect((x), 0) // could improve packing performance on your platform. Set to (x) to disable
#define STATSHOUSE_LIKELY(x) __builtin_expect((x), 1)

namespace statshouse {

#if __cplusplus >= 201703L

using string_view = std::string_view;

#else

class string_view {
  const char *d = nullptr;
  size_t s = 0;

public:
  string_view() = default;
  string_view(const char *str)
    : d(str)
    , s(std::strlen(str)) {}
  string_view(const std::string &str)
    : d(str.data())
    , s(str.size()) {}
  string_view(const char *d, size_t s)
    : d(d)
    , s(s) {}

  const char *data() const {
    return d;
  }
  size_t size() const {
    return s;
  }
  operator std::string() const {
    return std::string{d, s};
  }
};

#endif

namespace wyhash {
// https://github.com/wangyi-fudan/wyhash

// 128bit multiply function
static inline void _wymum(uint64_t *A, uint64_t *B) {
#if defined(__SIZEOF_INT128__)
  __uint128_t r = *A;
  r *= *B;
  *A = (uint64_t)r;
  *B = (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
  *A = _umul128(*A, *B, B);
#else
  uint64_t ha = *A >> 32, hb = *B >> 32, la = (uint32_t)*A, lb = (uint32_t)*B, hi, lo;
  uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + (rm0 << 32), c = t < rl;
  lo = t + (rm1 << 32);
  c += lo < t;
  hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
  *A = lo;
  *B = hi;
#endif
}

// multiply and xor mix function, aka MUM
static inline uint64_t _wymix(uint64_t A, uint64_t B) {
  _wymum(&A, &B);
  return A ^ B;
}

// read functions
static inline uint64_t _wyr8(const uint8_t *p) {
  uint64_t v;
  memcpy(&v, p, 8);
  return v;
}
static inline uint64_t _wyr4(const uint8_t *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}
static inline uint64_t _wyr3(const uint8_t *p, size_t k) {
  return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

// wyhash main function
static inline uint64_t wyhash(const void *key, size_t len, uint64_t seed, const uint64_t *secret) {
  const uint8_t *p = (const uint8_t *)key;
  seed ^= _wymix(seed ^ secret[0], secret[1]);
  uint64_t a, b;
  if (STATSHOUSE_LIKELY(len <= 16)) {
    if (STATSHOUSE_LIKELY(len >= 4)) {
      a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
      b = (_wyr4(p + len - 4) << 32) | _wyr4(p + len - 4 - ((len >> 3) << 2));
    } else if (STATSHOUSE_LIKELY(len > 0)) {
      a = _wyr3(p, len);
      b = 0;
    } else
      a = b = 0;
  } else {
    size_t i = len;
    if (STATSHOUSE_UNLIKELY(i > 48)) {
      uint64_t see1 = seed, see2 = seed;
      do {
        seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
        see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
        see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
        p += 48;
        i -= 48;
      } while (STATSHOUSE_UNLIKELY(i > 48));
      seed ^= see1 ^ see2;
    }
    while (STATSHOUSE_UNLIKELY(i > 16)) {
      seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
      i -= 16;
      p += 16;
    }
    a = _wyr8(p + i - 16);
    b = _wyr8(p + i - 8);
  }
  a ^= secret[1];
  b ^= seed;
  _wymum(&a, &b);
  return _wymix(a ^ secret[0] ^ len, b ^ secret[1]);
}

// the default secret parameters
static const uint64_t _wyp[4] = {0xa0761d6478bd642full, 0xe7037ed1a0b428dbull, 0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull};

} // namespace wyhash

struct nop_mutex { // will select lock policy later
  nop_mutex() noexcept = default;
  ~nop_mutex() = default;

  void lock() {}
  void unlock() {}
};

namespace test {
template<typename T>
struct traits;
}

class TransportUDPBase {
  friend class Registry;
  template<typename T>
  friend struct test::traits;

public:
  using mutex = nop_mutex; // for now use in experiments
  enum {
    DEFAULT_PORT = 13337,
    SAFE_DATAGRAM_SIZE = 508, // https://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
    DEFAULT_DATAGRAM_SIZE = 1232,
    MAX_DATAGRAM_SIZE = 65507, // https://stackoverflow.com/questions/42609561/udp-maximum-packet-size/42610200
    MAX_KEYS = 16,
    MAX_FULL_KEY_SIZE = 1024, // roughly metric plus all tags
  };

  // Metric name builder. Use by calling transport.metric("metric").tag("tag1").tag("tag2").write_count(1);
  // Contains reference to transport and must outlive it
  class MetricBuilder {
  public:
    // Assigns next tag value, starting from 1
    MetricBuilder &tag(string_view str) {
      return tag_id(next_tag++, str);
    }
    // Sets env (tag 0). If not set, transport's default_env will be used.
    MetricBuilder &env(string_view str) {
      env_set = true;
      return tag_id(0, str);
    }
    // Assigns arbitrary tag value by key name
    MetricBuilder &tag(string_view key, string_view str) {
      auto begin = buffer + buffer_pos;
      auto end = buffer + MAX_FULL_KEY_SIZE;
      if (STATSHOUSE_UNLIKELY(!(begin = pack_string(begin, end, key.data(), key.size())))) {
        did_not_fit = true;
        return *this;
      }
      if (STATSHOUSE_UNLIKELY(!(begin = pack_string(begin, end, str.data(), str.size())))) {
        did_not_fit = true;
        return *this;
      }
      buffer_pos = begin - buffer;
      tags_count++;
      return *this;
    }

    // for write_count. if writing with sample factor, set count to # of events before sampling
    bool write_count(double count, uint32_t tsUnixSec = 0) const {
      std::lock_guard<mutex> lo(transport->mu);
      return transport->write_count_impl(*this, count, tsUnixSec);
    }
    // for write_values. set count to # of events before sampling, values to sample of original values
    // if no sampling is performed, pass 0 (interpreted as values_count) to count
    bool write_values(const double *values, size_t values_count, double count = 0, uint32_t tsUnixSec = 0) const {
      std::lock_guard<mutex> lo(transport->mu);
      return transport->write_values_impl(*this, values, values_count, count, tsUnixSec);
    }
    bool write_value(double value, uint32_t tsUnixSec = 0) const {
      return write_values(&value, 1, 0, tsUnixSec);
    }
    // for write_unique, set count to # of events before sampling, values to sample of original hashes
    // for example, if you recorded events [1,1,1,1,2], you could pass them as is or as [1, 2] into 'values' and 5 into 'count'.
    bool write_unique(const uint64_t *values, size_t values_count, double count, uint32_t tsUnixSec = 0) const {
      std::lock_guard<mutex> lo(transport->mu);
      return transport->write_unique_impl(*this, values, values_count, count, tsUnixSec);
    }
    bool write_unique(uint64_t value, uint32_t tsUnixSec = 0) const {
      return write_unique(&value, 1, 1, tsUnixSec);
    }

  private:
    string_view metric_name() const {
      return {buffer + 1, metric_name_len};
    } // see pack_string
    string_view full_key_name() const {
      return {buffer, buffer_pos};
    }
    size_t tags_count_pos() const {
      return (1 + metric_name_len + 3) & ~3;
    } // see pack_string

    MetricBuilder &tag_id(size_t i, string_view str) {
      auto begin = buffer + buffer_pos;
      auto end = buffer + MAX_FULL_KEY_SIZE;
      if (STATSHOUSE_UNLIKELY(!(begin = pack32(begin, end, get_tag_name_tl(i))))) {
        did_not_fit = true;
        return *this;
      }
      if (STATSHOUSE_UNLIKELY(!(begin = pack_string(begin, end, str.data(), str.size())))) {
        did_not_fit = true;
        return *this;
      }
      buffer_pos = begin - buffer;
      tags_count++;
      return *this;
    }

    friend class Registry;
    friend class TransportUDPBase;
    explicit MetricBuilder(TransportUDPBase *transport, string_view m)
      : transport(transport) {
      // to simplify layout in buffer, we ensure metric name is in short format
      m = shorten(m);
      metric_name_len = m.size();
      static_assert(MAX_FULL_KEY_SIZE > 4 + TL_MAX_TINY_STRING_LEN + 4, "not enough space to not overflow in constructor");
      auto begin = buffer + buffer_pos;
      auto end = buffer + MAX_FULL_KEY_SIZE;
      begin = pack_string(begin, end, m.data(), m.size());
      begin = pack32(begin, end, 0); // place for tags_count. Never updated in buffer and always stays 0,
      buffer_pos = begin - buffer;
    }
    TransportUDPBase *transport;
    bool env_set = false;     // so current default_env will be added in pack_header even if set later.
    bool did_not_fit = false; // we do not want any exceptions in writing metrics
    size_t next_tag = 1;
    size_t tags_count = 0;
    size_t metric_name_len = 0;
    size_t buffer_pos = 0;          // not ptr because we want safe copy
    char buffer[MAX_FULL_KEY_SIZE]; // Uninitialized, due to performance considerations.
  };

  explicit TransportUDPBase() = default;
  virtual ~TransportUDPBase() = default;

  void set_default_env(const std::string &env) { // automatically sent as tag '0'
    std::lock_guard<mutex> lo(mu);
    default_env = env;
    if (default_env.empty()) {
      default_env_tl_pair.clear();
      return;
    }

    default_env_tl_pair.resize(4 + default_env.size() + 4); // key name, env, padding
    auto begin = &default_env_tl_pair[0];
    auto end = begin + default_env_tl_pair.size();

    begin = pack32(begin, end, pack_key_name(0));
    begin = pack_string(begin, end, default_env.data(), default_env.size());

    default_env_tl_pair.resize(begin - &default_env_tl_pair[0]);
  }
  std::string get_default_env() const {
    std::lock_guard<mutex> lo(mu);
    return default_env;
  }

  MetricBuilder metric(string_view name) {
    return MetricBuilder(this, name);
  }

  // flush() is performed automatically every time write_* is called, but in most services
  // there might be extended period, when no metrics are written, so it is recommended to
  // call flush() 1-10 times per second so no metric will be stuck in a buffer for a long time.
  void flush(bool force = false) {
    std::lock_guard<mutex> lo(mu);
    auto now = time_now();
    if (force) {
      flush_impl(now);
    } else {
      maybe_flush(now);
    }
  }

  // if you call this function, transport will stop calling std::time() function forever (so will be slightly faster),
  // but you promise to call set_external_time forever every time clock second changes.
  // You can call it as often as you like, for example on each event loop invocation before calling user code,
  // so every event is marked by exact timestamp it happened.
  // If you decide to call set_external_time, do not call flush()
  void set_external_time(uint32_t timestamp) {
    std::lock_guard<mutex> lo(mu); // not perfect, make packet_ts atomic
    if (STATSHOUSE_UNLIKELY(tick_external && timestamp == packet_ts)) {
      return;
    }
    tick_external = true;
    flush_impl(timestamp);
  }

  // max packet size can be changed at any point in instance lifetime
  // by default on localhost packet_size will be set to MAX_DATAGRAM_SIZE, otherwise to DEFAULT_DATAGRAM_SIZE
  void set_max_udp_packet_size(size_t s) {
    std::lock_guard<mutex> lo(mu);
    max_payload_size = std::max<size_t>(SAFE_DATAGRAM_SIZE, std::min<size_t>(s, MAX_DATAGRAM_SIZE));
  }

  // If set, will flush packet after each metric. Slow and unnecessary. Only for testing.
  void set_immediate_flush(bool f) {
    std::lock_guard<mutex> lo(mu);
    immediate_flush = f;
    flush_impl(time_now());
  }

  struct Stats {
    size_t metrics_sent = 0;
    size_t metrics_overflow = 0;
    size_t metrics_failed = 0;
    size_t metrics_odd_kv = 0;
    size_t metrics_too_big = 0;
    size_t packets_sent = 0;
    size_t packets_overflow = 0;
    size_t packets_failed = 0;
    size_t bytes_sent = 0;
    std::string last_error;
  };
  Stats get_stats() const {
    std::lock_guard<mutex> lo(mu);
    Stats other = stats;
    return other;
  }
  void clear_stats() {
    std::lock_guard<mutex> lo(mu);
    stats = Stats{};
  }

  // writes and clears per metric counters to meta metric STATSHOUSE_USAGE_METRICS
  // status "ok" is written always, error statuses only if corresponding counter != 0
  bool write_usage_metrics(string_view project, string_view cluster) {
    std::lock_guard<mutex> lo(mu);
    auto result = true;
    result = write_usage_metric_impl(project, cluster, "ok", &stats.metrics_sent, true) && result;
    result = write_usage_metric_impl(project, cluster, "err_sendto_would_block", &stats.metrics_overflow, false) && result;
    result = write_usage_metric_impl(project, cluster, "err_sendto_other", &stats.metrics_failed, false) && result;
    result = write_usage_metric_impl(project, cluster, "err_odd_kv", &stats.metrics_odd_kv, false) && result;
    result = write_usage_metric_impl(project, cluster, "err_header_too_big", &stats.metrics_too_big, false) && result;
    return result;
  }

  static std::string version() {
    return STATSHOUSE_TRANSPORT_VERSION;
  }

protected:
  Stats stats; // allow implementations to update stats directly for simplicity
private:
  enum {
    TL_INT_SIZE = 4,
    TL_LONG_SIZE = 8,
    TL_DOUBLE_SIZE = 8,
    TL_MAX_TINY_STRING_LEN = 253,
    TL_BIG_STRING_LEN = 0xffffff,
    TL_BIG_STRING_MARKER = 0xfe,
    TL_STATSHOUSE_METRICS_BATCH_TAG = 0x56580239,
    TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK = 1 << 0,
    TL_STATSHOUSE_METRIC_TS_FIELDS_MASK = 1 << 4,
    TL_STATSHOUSE_METRIC_VALUE_FIELDS_MASK = 1 << 1,
    TL_STATSHOUSE_METRIC_UNIQUE_FIELDS_MASK = 1 << 2,
    BATCH_HEADER_LEN = TL_INT_SIZE * 3 // TL tag, fields_mask, # of batches
  };
  mutable mutex mu;
  size_t batch_size = 0;                // we fill packet header before sending
  size_t packet_len = BATCH_HEADER_LEN; // reserve space for metric batch header

  std::string default_env;
  std::string default_env_tl_pair; // "0", default_env in TL format to optimized writing

  size_t max_payload_size = DEFAULT_DATAGRAM_SIZE;
  bool immediate_flush = false;
  bool tick_external = false;
  uint32_t packet_ts = 0; // also serves as external clock

  char packet[MAX_DATAGRAM_SIZE]{}; // zeroing is cheap, we are cautious

  uint32_t time_now() const {
    return tick_external ? packet_ts : static_cast<uint32_t>(std::time(nullptr));
  }
  static void put32(char *buf, uint32_t val) { // optimized to mov by modern compiler
                                               //		std::memcpy(buf, &val, 4); // endian dependent, hence commented, code below is equally fast
    buf[0] = char(val);
    buf[1] = char(val >> 8);
    buf[2] = char(val >> 16);
    buf[3] = char(val >> 24);
  }
  static void put64(char *buf, uint64_t val) { // optimized to mov by modern compiler
                                               //		std::memcpy(buf, &val, 8); // endian dependent, hence commented, code below is equally fast
    buf[0] = char(val);
    buf[1] = char(val >> 8);
    buf[2] = char(val >> 16);
    buf[3] = char(val >> 24);
    buf[4] = char(val >> 32);
    buf[5] = char(val >> 40);
    buf[6] = char(val >> 48);
    buf[7] = char(val >> 56);
  }
  static bool enoughSpace(const char *begin, const char *end, size_t req) {
    return begin + req <= end;
  }
  static char *pack32(char *begin, const char *end, size_t v) {
    if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, 4))) {
      return nullptr;
    }
    put32(begin, uint32_t(v));
    return begin + 4;
  }
  static char *pack64(char *begin, const char *end, uint64_t v) {
    if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, 8))) {
      return nullptr;
    }
    put64(begin, v);
    return begin + 8;
  }
  static uint64_t doubleBits(double v) {
    uint64_t v64 = 0;
    static_assert(sizeof(v) == sizeof v64, "Please make ifdef here with code writing IEEE 64-bit LE double for your (exotic?) platform");
    std::memcpy(&v64, &v, sizeof(v));
    return v64;
  }
  static string_view shorten(string_view x) {
    return string_view{x.data(), std::min<size_t>(x.size(), TL_MAX_TINY_STRING_LEN)};
  }
  //	Trim is VERY costly, 2x slowdown even when no actual trim performed. We do not want to punish good guys, and for bad guys
  //		we have 'err_header_too_big' usage meta metric
  //	static char * pack_string_trim(char * begin, const char * end, const char *str, size_t len) {
  //		while (STATSHOUSE_UNLIKELY(len > 0 && std::isspace(static_cast<unsigned char>(*str)))) {
  //			++str;
  //			--len;
  //		}
  //		return pack_string(begin, end, str, len);
  //	}
  static constexpr uint32_t pack_key_name(size_t i) {
    static_assert(MAX_KEYS <= 100, "key names up to 100 supported");
    return (i < 10) ? (1 | (uint32_t('0' + i) << 8)) : (2 | (uint32_t('0' + i / 10) << 8) | (uint32_t('0' + i % 10) << 16));
  }
  static uint32_t get_tag_name_tl(size_t i) {
    static constexpr std::array<uint32_t, MAX_KEYS + 1> names{
      pack_key_name(0),  pack_key_name(1),  pack_key_name(2),  pack_key_name(3),  pack_key_name(4),  pack_key_name(5),
      pack_key_name(6),  pack_key_name(7),  pack_key_name(8),  pack_key_name(9),  pack_key_name(10), pack_key_name(11),
      pack_key_name(12), pack_key_name(13), pack_key_name(14), pack_key_name(15), pack_key_name(16),
      // we add extra key on purpose. statshouse will show receive error "tag 16 does not exist" if we overflow
    };
    static_assert(names[MAX_KEYS] != 0, "please add key names to array when increasing MAX_KEYS");
    if (i > names.size())
      return 0; // if even more tags added, we do not care, so empty string is good enough.
    return names[i];
  }
  static char *pack_string(char *begin, const char *end, const char *str, size_t len) {
    if (STATSHOUSE_UNLIKELY(len > TL_MAX_TINY_STRING_LEN)) {
      if (STATSHOUSE_UNLIKELY(len > TL_BIG_STRING_LEN)) {
        len = TL_BIG_STRING_LEN;
      }
      auto fullLen = (4 + len + 3) & ~3;
      if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, fullLen))) {
        return nullptr;
      }
      put32(begin + fullLen - 4, 0); // padding first
      put32(begin, (len << 8U) | TL_BIG_STRING_MARKER);
      std::memcpy(begin + 4, str, len);
      begin += fullLen;
    } else {
      auto fullLen = (1 + len + 3) & ~3;
      if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, fullLen))) {
        return nullptr;
      }
      put32(begin + fullLen - 4, 0);   // padding first
      *begin = static_cast<char>(len); // or put32(p, len);
      std::memcpy(begin + 1, str, len);
      begin += fullLen;
    }
    return begin;
  }
  char *pack_header(uint32_t now, size_t min_space, const MetricBuilder &metric, double counter, uint32_t tsUnixSec, size_t fields_mask) {
    if (STATSHOUSE_UNLIKELY(metric.did_not_fit)) {
      stats.last_error.clear(); // prevent allocations
      stats.last_error.append("statshouse::TransportUDP header too big for metric=");
      stats.last_error.append(metric.metric_name());
      ++stats.metrics_too_big;
      return nullptr;
    }
    if (tsUnixSec == 0) {
      tsUnixSec = now;
    }
    char *begin = packet + packet_len;
    const char *end = packet + max_payload_size;
    if ((begin = pack_header_impl(begin, end, metric, counter, tsUnixSec, fields_mask)) && enoughSpace(begin, end, min_space)) {
      return begin;
    }
    if (packet_len != BATCH_HEADER_LEN) {
      flush_impl(now);
      begin = packet + packet_len;
      if ((begin = pack_header_impl(begin, end, metric, counter, tsUnixSec, fields_mask)) && enoughSpace(begin, end, min_space)) {
        return begin;
      }
    }
    stats.last_error.clear(); // prevent allocations
    stats.last_error.append("statshouse::TransportUDP header too big for metric=");
    stats.last_error.append(metric.metric_name());
    ++stats.metrics_too_big;
    return nullptr;
  }
  char *pack_header_impl(char *begin, const char *end, const MetricBuilder &metric, double counter, uint32_t tsUnixSec, size_t fields_mask) {
    if (tsUnixSec != 0) {
      fields_mask |= TL_STATSHOUSE_METRIC_TS_FIELDS_MASK;
    }
    if (STATSHOUSE_UNLIKELY(!(begin = pack32(begin, end, fields_mask)))) {
      return nullptr;
    }
    if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, metric.buffer_pos))) {
      return nullptr;
    }
    std::memcpy(begin, metric.buffer, metric.buffer_pos);
    const bool add_env = !metric.env_set && !default_env_tl_pair.empty();
    put32(begin + metric.tags_count_pos(), metric.tags_count + int(add_env));
    begin += metric.buffer_pos;
    if (add_env) {
      if (STATSHOUSE_UNLIKELY(!enoughSpace(begin, end, default_env_tl_pair.size()))) {
        return nullptr;
      }
      std::memcpy(begin, default_env_tl_pair.data(), default_env_tl_pair.size());
      begin += default_env_tl_pair.size();
    }
    if (fields_mask & TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK) {
      if (STATSHOUSE_UNLIKELY(!(begin = pack64(begin, end, doubleBits(counter))))) {
        return nullptr;
      }
    }
    if (fields_mask & TL_STATSHOUSE_METRIC_TS_FIELDS_MASK) {
      if (STATSHOUSE_UNLIKELY(!(begin = pack32(begin, end, tsUnixSec)))) {
        return nullptr;
      }
    }
    return begin;
  }
  bool write_count_impl(const MetricBuilder &metric, double count, uint32_t tsUnixSec) {
    auto now = time_now();
    char *begin = pack_header(now, 0, metric, count, tsUnixSec, TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK);
    if (!begin) {
      return false; // did not fit into empty buffer
    }
    packet_len = begin - packet;
    ++batch_size;
    maybe_flush(now);
    return true;
  }
  bool write_values_impl(const MetricBuilder &metric, const double *values, size_t values_count, double count, uint32_t tsUnixSec) {
    size_t fields_mask = TL_STATSHOUSE_METRIC_VALUE_FIELDS_MASK;
    if (count != 0 && count != double(values_count)) {
      fields_mask |= TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK;
    }
    auto now = time_now();
    const char *end = packet + max_payload_size;
    while (values_count != 0) {
      char *begin = pack_header(now, TL_INT_SIZE + TL_DOUBLE_SIZE, metric, count, tsUnixSec, fields_mask);
      if (!begin) {
        return false; // did not fit into empty buffer
      }
      auto write_count = std::min<size_t>(values_count, (end - begin - TL_INT_SIZE) / TL_DOUBLE_SIZE); // at least 1
      put32(begin, write_count);
      begin += TL_INT_SIZE;
      for (size_t j = 0; j != write_count; ++j) {
        put64(begin + j * TL_DOUBLE_SIZE, doubleBits(values[j]));
      }
      values += write_count;
      values_count -= write_count;
      packet_len = begin + write_count * TL_DOUBLE_SIZE - packet;
      ++batch_size;
    }
    maybe_flush(now);
    return true;
  }
  bool write_unique_impl(const MetricBuilder &metric, const uint64_t *values, size_t values_count, double count, uint32_t tsUnixSec) {
    size_t fields_mask = TL_STATSHOUSE_METRIC_UNIQUE_FIELDS_MASK;
    if (count != 0 && count != double(values_count)) {
      fields_mask |= TL_STATSHOUSE_METRIC_COUNTER_FIELDS_MASK;
    }
    auto now = time_now();
    const char *end = packet + max_payload_size;
    while (values_count != 0) {
      char *begin = pack_header(now, TL_INT_SIZE + TL_LONG_SIZE, metric, count, tsUnixSec, fields_mask);
      if (!begin) {
        return false; // did not fit into empty buffer
      }
      auto write_count = std::min<size_t>(values_count, (end - begin - TL_INT_SIZE) / TL_LONG_SIZE); // at least 1
      put32(begin, write_count);
      begin += TL_INT_SIZE;
      for (size_t j = 0; j != write_count; ++j) {
        put64(begin + j * TL_LONG_SIZE, values[j]);
      }
      values += write_count;
      values_count -= write_count;
      packet_len = begin + write_count * TL_LONG_SIZE - packet;
      ++batch_size;
    }
    maybe_flush(now);
    return true;
  }
  bool write_usage_metric_impl(string_view project, string_view cluster, string_view status, size_t *value, bool send_if_0) {
    if (*value || send_if_0) {
      auto count = double(*value);
      *value = 0;
      write_count_impl(metric(STATSHOUSE_USAGE_METRICS)
                         .tag("status", status)
                         .tag("project", project)
                         .tag("cluster", cluster)
                         .tag("protocol", "udp")
                         .tag("language", "cpp")
                         .tag("version", STATSHOUSE_TRANSPORT_VERSION),
                       count, 0);
    }
    return true;
  }
  void maybe_flush(uint32_t now) {
    if (immediate_flush || now != packet_ts) {
      flush_impl(now);
    }
  }
  virtual bool on_flush_packet(const char *data, size_t size, size_t batch_size) = 0;
  void flush_impl(uint32_t now) {
    packet_ts = now;
    if (batch_size == 0) {
      return;
    }
    put32(packet, TL_STATSHOUSE_METRICS_BATCH_TAG);
    put32(packet + TL_INT_SIZE, 0);                        // fields mask
    put32(packet + 2 * TL_INT_SIZE, uint32_t(batch_size)); // batch size

    auto result = on_flush_packet(packet, packet_len, batch_size);
    if (result) {
      ++stats.packets_sent;
      stats.metrics_sent += batch_size;
      stats.bytes_sent += packet_len;
    } else {
      ++stats.packets_failed;
      stats.metrics_failed += batch_size;
    }
    // we will lose usage metrics if packet with usage is discarded, but we are ok with that
    batch_size = 0;
    packet_len = BATCH_HEADER_LEN;
  }
};

// Use #ifdefs to customize for various platforms here
class TransportUDP : public TransportUDPBase {
  template<typename T>
  friend struct test::traits;

public:
  // no functions of this class throw
  // pass empty ip to use as dummy writer
  // access last error and statistics with move_stats()
  // all functions return false in case error happened, you can ignore them or write last error to log periodically
  // in multithread environment use external lock to access this instance
  TransportUDP()
    : TransportUDP("127.0.0.1", DEFAULT_PORT) {}
  TransportUDP(const std::string &ip, int port)
    : dummy_instance(ip.empty() || port == 0) {
    if (!dummy_instance) {
      udp_socket = create_socket(ip, port);
    }
  }
  TransportUDP(const TransportUDP &) = delete;
  TransportUDP &operator=(const TransportUDP &) = delete;
  ~TransportUDP() override {
    flush(true);
    (void)::close(udp_socket);
  }
  bool is_socket_valid() const {
    return udp_socket >= 0;
  }

  static int test_main() { // call from your main for testing
    TransportUDP statshouse;

    statshouse.set_default_env("production");

    statshouse.metric("toy").tag("android").tag("count").write_count(7);
    std::vector<double> values{1, 2, 3};
    statshouse.metric("toy").tag("android").tag("values").write_values(values.data(), values.size(), 6);
    std::vector<uint64_t> uniques{1, 2, 3};
    statshouse.metric("toy").tag("android").tag("uniques").env("staging").write_unique(uniques.data(), uniques.size(), 5, 1630000000);

    statshouse.metric("toy").tag("platform", "android").tag("2", "count_kv").write_count(1);

    statshouse.write_usage_metrics("test_main", "toy");
    return 0;
  }
  static size_t benchmark_pack_header(size_t total_size) {
    TransportUDP tmp("", 0);
    tmp.set_default_env("production");
    tmp.set_max_udp_packet_size(MAX_DATAGRAM_SIZE);
    tmp.set_external_time(std::time(nullptr));

    std::vector<std::string> dynamic_tags;
    while (dynamic_tags.size() < 1000000) {
      dynamic_tags.push_back("tag3" + std::to_string(dynamic_tags.size()));
    }
    size_t next_tag_value = 0;

    while (tmp.stats.bytes_sent < total_size) {
      // when all tags are static, this compiles into setting metric memory to fixed bytes corresponding to TL representation of strings
      tmp.metric("typical_metric_name").tag("tag1_name").tag("tag2_name").tag(dynamic_tags[next_tag_value]).tag("tag4_name").write_count(1);
      if (++next_tag_value >= dynamic_tags.size())
        next_tag_value = 0;
    }

    return tmp.stats.metrics_sent;
  }

private:
  int udp_socket = -1;
  bool dummy_instance = false; // better than separate class, because can be set depending on config

  bool on_flush_packet(const char *data, size_t size, size_t batch_size) override {
    if (dummy_instance) {
      return true;
    }
    if (!is_socket_valid()) { // we reported connection error after start
      return false;
    }

    ssize_t result = ::sendto(udp_socket, data, size, MSG_DONTWAIT, nullptr, 0);

    if (result >= 0) {
      return true;
    }
    auto err = errno;
    if (err == EAGAIN) { //  || err == EWOULDBLOCK
      ++stats.packets_overflow;
      stats.metrics_overflow += batch_size;
      return false;
    }
    set_errno_error(err, "statshouse::TransportUDP sendto() failed");
    return false;
  }

  int create_socket(const std::string &ip, int port) {
    if (port < 0 || port > 0xffff) {
      stats.last_error = "statshouse::TransportUDP invalid port=" + std::to_string(port);
      return -1;
    }
    ::sockaddr_storage addr = {};
    auto ap = reinterpret_cast<sockaddr *>(&addr);
    auto ap6 = reinterpret_cast<sockaddr_in6 *>(ap);
    auto ap4 = reinterpret_cast<sockaddr_in *>(ap);
    int ap_len = 0;
    if (inet_pton(AF_INET6, ip.c_str(), &ap6->sin6_addr) == 1) {
      addr.ss_family = AF_INET6;
      ap6->sin6_port = htons(uint16_t(port));
      ap_len = sizeof(*ap6);
      // TODO - check if localhost and increase default packet size
    } else if (inet_pton(AF_INET, ip.c_str(), &ap4->sin_addr) == 1) {
      addr.ss_family = AF_INET;
      ap4->sin_port = htons(uint16_t(port));
      ap_len = sizeof(*ap4);
      char high_byte = 0;
      std::memcpy(&high_byte, &ap4->sin_addr, 1); // this is correct, sin_addr in network byte order
      if (high_byte == 0x7F) {
        set_max_udp_packet_size(MAX_DATAGRAM_SIZE);
      }
    } else {
      stats.last_error = "statshouse::TransportUDP could not parse ip=" + ip;
      return -1;
    }
    auto sock = ::socket(addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
      set_errno_error(errno, "statshouse::TransportUDP socket() failed");
      return -1;
    }
    if (::connect(sock, ap, ap_len) < 0) {
      ::close(sock);
      set_errno_error(errno, "statshouse::TransportUDP connect() failed");
      return -1;
    }
    return sock;
  }
  void set_errno_error(int err, const char *msg) {
    stats.last_error.clear(); // prevent allocations
    stats.last_error.append(msg);
    stats.last_error.append(" errno=");
    stats.last_error.append(std::to_string(err));
    stats.last_error.append(", ");
    stats.last_error.append(strerror(err));
  }
};

class Registry {
  template<typename T>
  friend struct test::traits;
  enum {
    DEFAULT_MAX_BUCKET_SIZE = 1024,
    EXPLICIT_FLUSH_CHUNK_SIZE = 10,
    INCREMENTAL_FLUSH_SIZE = 2,
  };

public:
  Registry()
    : Registry("127.0.0.1", TransportUDP::DEFAULT_PORT) {}
  Registry(const std::string &host, int port)
    : max_bucket_size{DEFAULT_MAX_BUCKET_SIZE}
    , time_external{0}
    , incremental_flush_disabled{false}
    , transport{host, port} {}
  Registry(const Registry &other) = delete;
  Registry(Registry &&other) = delete;
  Registry &operator=(const Registry &other) = delete;
  Registry &operator=(Registry &&other) = delete;
  ~Registry() {
    flush(true);
  }

private:
  struct multivalue_view {
    explicit multivalue_view(double count)
      : count{count}
      , values_count{0}
      , values{nullptr}
      , unique{nullptr} {}
    multivalue_view(double count, size_t values_count, const double *values)
      : count{std::max(count, static_cast<double>(values_count))}
      , values_count{values_count}
      , values{values}
      , unique{nullptr} {}
    multivalue_view(double count, size_t values_count, const uint64_t *unique)
      : count{std::max(count, static_cast<double>(values_count))}
      , values_count{values_count}
      , values{nullptr}
      , unique{unique} {}
    size_t empty() const {
      return count == 0 && !values_count;
    }
    double count;
    size_t values_count;
    const double *values;
    const uint64_t *unique;
  };
  struct multivalue {
    void write(multivalue_view &src, size_t limit) {
      if (src.values) {
        write(values, src.values, src.values_count, src.count, limit);
      } else if (src.unique) {
        write(unique, src.unique, src.values_count, src.count, limit);
      } else {
        count += src.count;
        src.count = 0;
      }
    }
    template<typename T>
    void write(std::vector<T> &dst, const T *src, size_t &src_size, double &src_count, size_t limit) {
      if (!src_size || limit <= dst.size()) {
        return;
      }
      // limit size & count
      auto effective_size = std::min(src_size, limit - dst.size());
      auto effective_count = (effective_size / src_size) * src_count;
      // update this
      dst.insert(dst.end(), src, src + effective_size);
      count += effective_count;
      // update arguments
      src_size -= effective_size;
      src_count -= effective_count;
    }
    bool empty() const {
      return count == 0 && values.empty() && unique.empty();
    }
    double count{};
    std::vector<double> values;
    std::vector<uint64_t> unique;
  };
  struct bucket {
    bucket(const TransportUDP::MetricBuilder &key, uint32_t timestamp)
      : key{key}
      , timestamp{timestamp}
      , queue_ptr{nullptr} {}
    TransportUDP::MetricBuilder key;
    uint32_t timestamp;
    multivalue value;
    std::mutex mu;
    std::shared_ptr<bucket> *queue_ptr;
  };

public:
  class MetricRef {
  public:
    MetricRef(Registry *registry, const TransportUDP::MetricBuilder &key)
      : registry{registry}
      , ptr{registry->get_or_create_bucket(key)} {}
    bool write_count(double count, uint32_t timestamp = 0) const {
      if (timestamp) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return ptr->key.write_count(count, timestamp);
      }
      multivalue_view value{count};
      return registry->update_multivalue_by_ref(ptr, value);
    }
    bool write_value(double value, uint32_t timestamp = 0) const {
      return write_values(&value, 1, 1, timestamp);
    }
    bool write_values(const double *values, size_t values_count, double count = 0, uint32_t timestamp = 0) const {
      if (timestamp || values_count >= registry->max_bucket_size.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return ptr->key.write_values(values, values_count, count, timestamp);
      }
      multivalue_view value{count, values_count, values};
      return registry->update_multivalue_by_ref(ptr, value);
    }
    bool write_unique(uint64_t value, uint32_t timestamp = 0) const {
      return write_unique(&value, 1, 1, timestamp);
    }
    bool write_unique(const uint64_t *values, size_t values_count, double count, uint32_t timestamp = 0) const {
      if (timestamp || values_count >= registry->max_bucket_size.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return ptr->key.write_unique(values, values_count, count, timestamp);
      }
      multivalue_view value{count, values_count, values};
      return registry->update_multivalue_by_ref(ptr, value);
    }

  private:
    Registry *registry;
    std::shared_ptr<bucket> ptr;
  };
  class MetricBuilder {
  public:
    MetricBuilder(Registry *registry, string_view name)
      : registry{registry}
      , key{&registry->transport, name} {}
    MetricBuilder &tag(string_view str) {
      key.tag(str);
      return *this;
    }
    MetricBuilder &env(string_view str) {
      key.env(str);
      return *this;
    }
    MetricBuilder &tag(string_view a, string_view b) {
      key.tag(a, b);
      return *this;
    }
    MetricRef ref() const {
      return MetricRef{registry, key};
    }
    bool write_count(double count, uint32_t timestamp = 0) const {
      if (timestamp) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return key.write_count(count, timestamp);
      }
      multivalue_view value{count};
      return registry->update_multivalue_by_key(key, value);
    }
    bool write_value(double value, uint32_t timestamp = 0) const {
      return write_values(&value, 1, 1, timestamp);
    }
    bool write_values(const double *values, size_t values_count, double count = 0, uint32_t timestamp = 0) const {
      if (timestamp || values_count >= registry->max_bucket_size.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return key.write_values(values, values_count, count, timestamp);
      }
      multivalue_view value{count, values_count, values};
      return registry->update_multivalue_by_key(key, value);
    }
    bool write_unique(uint64_t value, uint32_t timestamp = 0) const {
      return write_unique(&value, 1, 1, timestamp);
    }
    bool write_unique(const uint64_t *values, size_t values_count, double count, uint32_t timestamp = 0) const {
      if (timestamp || values_count >= registry->max_bucket_size.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> transport_lock{registry->transport_mu};
        return key.write_unique(values, values_count, count, timestamp);
      }
      multivalue_view value{count, values_count, values};
      return registry->update_multivalue_by_key(key, value);
    }

  private:
    Registry *registry;
    TransportUDP::MetricBuilder key;
  };
  MetricBuilder metric(string_view name) {
    return {this, name};
  }
  // If called once, then you need to call it until the end of the life of the registry.
  void set_external_time(uint32_t timestamp) {
    time_external = timestamp;
  }
  // Call once during initialization, if set then you are responsible for calling "flush".
  void disable_incremental_flush() {
    incremental_flush_disabled = true;
  }
  void flush(bool force = false) {
    if (force) {
      flush(time_now());
    } else if (incremental_flush_disabled) {
      flush(time_now() - 1);
    } else {
      flush(time_now() - 2);
    }
  }
  void set_max_udp_packet_size(size_t n) {
    std::lock_guard<std::mutex> transport_lock{transport_mu};
    transport.set_max_udp_packet_size(n);
  }
  void set_max_bucket_size(size_t n) {
    max_bucket_size.store(n, std::memory_order_relaxed);
  }
  void set_default_env(const std::string &env) {
    std::lock_guard<std::mutex> transport_lock{transport_mu};
    transport.set_default_env(env);
  }
  class Clock {
  public:
    explicit Clock(Registry *registry)
      : thread{start(registry)} {}
    ~Clock() {
      stop = true;
      thread.join();
    }

  private:
    std::thread start(Registry *registry) {
      registry->set_external_time(std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count());
      return std::thread(&Clock::run, this, registry);
    }
    void run(Registry *registry) {
      while (!stop) {
        auto atime = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()) + std::chrono::seconds{1};
        std::this_thread::sleep_until(atime);
        if (stop) {
          break;
        }
        registry->set_external_time(atime.time_since_epoch().count());
      }
    }
    std::thread thread;
    std::atomic_bool stop{};
  };

private:
  bool update_multivalue_by_key(const TransportUDP::MetricBuilder &key, multivalue_view &value) {
    auto timestamp = time_now();
    return update_multivalue_by_ref(get_or_create_bucket(key, timestamp), timestamp, value);
  }
  bool update_multivalue_by_ref(const std::shared_ptr<bucket> &ptr, multivalue_view &value) {
    return update_multivalue_by_ref(ptr, time_now(), value);
  }
  bool update_multivalue_by_ref(const std::shared_ptr<bucket> &ptr, uint32_t timestamp, multivalue_view &value) {
    auto limit = max_bucket_size.load(std::memory_order_relaxed);
    {
      std::lock_guard<std::mutex> bucket_lock{ptr->mu};
      if (timestamp <= ptr->timestamp) {
        ptr->value.write(value, limit);
        if (value.empty()) {
          return true; // fast path
        }
        // protect against clock going backwards before falling through
        timestamp = ptr->timestamp;
      } else {
        // next second
      }
    }
    // split
    size_t queue_size = 0;
    {
      std::lock_guard<std::mutex> lock{mu};
      queue.emplace_back(std::move(*ptr->queue_ptr));
      *ptr->queue_ptr = alloc_bucket(ptr->key, ptr->timestamp);
      {
        std::lock_guard<std::mutex> bucket_lock{ptr->mu};
        std::swap((*ptr->queue_ptr)->value, ptr->value);
        ptr->timestamp = timestamp;
        ptr->queue_ptr = &queue.back();
        ptr->value.write(value, limit);
        // assert(value.empty())
      }
      queue_size = queue.size();
    }
    if (incremental_flush_disabled) {
      return true;
    }
    // flush at most INCREMENTAL_FLUSH_SIZE buckets, don't flush bucket just added
    std::array<std::shared_ptr<bucket>, INCREMENTAL_FLUSH_SIZE> buffer;
    flush_some(buffer.data(), std::min(queue_size - 1, buffer.size()));
    return true;
  }
  void flush(uint32_t timestamp) {
    std::array<std::shared_ptr<bucket>, EXPLICIT_FLUSH_CHUNK_SIZE> buffer;
    while (flush_some(buffer.data(), buffer.size(), timestamp)) {
      // pass
    }
  }
  bool flush_some(std::shared_ptr<bucket> *buffer, size_t size, uint32_t timestamp = std::numeric_limits<uint32_t>::max()) {
    size_t count = 0;
    {
      std::lock_guard<std::mutex> lock{mu};
      for (; count < size && !queue.empty() && queue.front()->timestamp <= timestamp;) {
        auto &ptr = queue.front();
        auto keep = false;
        {
          std::lock_guard<std::mutex> bucket_lock{ptr->mu};
          if (!ptr->value.empty()) {
            buffer[count] = alloc_bucket(ptr->key, ptr->timestamp);
            std::swap(buffer[count]->value, ptr->value);
            ++count;
            keep = true;
          }
        }
        // pop front
        if (keep || ptr.use_count() > 2) {
          // either there are clients holding bucket reference
          // or bucket was written recently, move bucket back
          ptr->timestamp = time_now();
          queue.push_back(std::move(ptr));
          queue.back()->queue_ptr = &queue.back();
        } else {
          // removing bucket with non-zero "queue_ptr" implies removal from dictionary
          if (ptr->queue_ptr) {
            ptr->queue_ptr = nullptr;
            buckets.erase(string_view{ptr->key.buffer, ptr->key.buffer_pos});
          }
          // unused bucket goes into freelist
          freelist.emplace_back(std::move(ptr));
        }
        queue.pop_front();
      }
    }
    for (size_t i = 0; i < count; ++i) {
      auto &ptr = buffer[i];
      {
        std::lock_guard<std::mutex> transport_lock{transport_mu};
        auto &v = ptr->value;
        if (!v.values.empty()) {
          ptr->key.write_values(v.values.data(), v.values.size(), v.count, timestamp);
        } else if (!v.unique.empty()) {
          ptr->key.write_unique(v.unique.data(), v.unique.size(), v.count, timestamp);
        } else if (v.count != 0) {
          ptr->key.write_count(v.count, timestamp);
        }
        v.count = 0;
        v.values.clear();
        v.unique.clear();
      }
      std::lock_guard<std::mutex> lock{mu};
      freelist.emplace_back(std::move(ptr));
    }
    return count == size;
  }
  std::shared_ptr<bucket> get_or_create_bucket(const TransportUDP::MetricBuilder &key, uint32_t timestamp = 0) {
    std::lock_guard<std::mutex> lock{mu};
    auto it = buckets.find(string_view{key.buffer, key.buffer_pos});
    if (it != buckets.end()) {
      return it->second;
    }
    auto ptr = alloc_bucket(key, timestamp ? timestamp : time_now());
    queue.emplace_back(buckets.emplace(string_view{ptr->key.buffer, ptr->key.buffer_pos}, ptr).first->second);
    ptr->queue_ptr = &queue.back();
    return ptr;
  }
  std::shared_ptr<bucket> alloc_bucket(const TransportUDP::MetricBuilder &key, uint32_t timestamp) {
    if (freelist.empty()) {
      return std::make_shared<bucket>(key, timestamp);
    }
    auto ptr = std::move(freelist.back());
    ptr->key = key;
    ptr->timestamp = timestamp;
    freelist.pop_back();
    return ptr;
  }
  uint32_t time_now() const {
    uint32_t t{time_external};
    return t != 0 ? t : static_cast<uint32_t>(std::time(nullptr));
  }
  struct string_view_hash {
    size_t operator()(string_view a) const {
      return wyhash::wyhash(a.data(), a.size(), 0, wyhash::_wyp);
    }
  };
  struct string_view_equal {
    bool operator()(string_view a, string_view b) const {
      return a.size() == b.size() && !std::memcmp(a.data(), b.data(), a.size());
    }
  };
  std::mutex mu;
  std::mutex transport_mu;
  std::atomic<size_t> max_bucket_size;
  std::atomic<uint32_t> time_external;
  bool incremental_flush_disabled;
  TransportUDP transport;
  std::deque<std::shared_ptr<bucket>> queue;
  std::vector<std::shared_ptr<bucket>> freelist;
  std::unordered_map<string_view, std::shared_ptr<bucket>, string_view_hash, string_view_equal> buckets;
};

} // namespace statshouse

#undef STATSHOUSE_UNLIKELY
#undef STATSHOUSE_USAGE_METRIC
#undef STATSHOUSE_TRANSPORT_VERSION
