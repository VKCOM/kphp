// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/vkext/vkext_stats.h"

#include <cassert>
#include <cstring>

#include "runtime-common/core/runtime-core.h"

constexpr char HLL_FIRST_RANK_CHAR = 0x30;
constexpr char HLL_PACK_CHAR = '!';
constexpr char HLL_PACK_CHAR_V2 = '$';
constexpr size_t MAX_HLL_SIZE = 1 << 14;
constexpr size_t HLL_BUF_SIZE = MAX_HLL_SIZE + 1000;

static constexpr char to_half_byte(char c) noexcept {
  return ((c > '9') ? (c - 7) : c) - '0';
}

//////
//    hll functions
//////

static bool is_hll_unpacked(const string &hll) noexcept {
  return hll.empty() || (hll[0] != HLL_PACK_CHAR && hll[0] != HLL_PACK_CHAR_V2);
}

static int get_hll_size(const string &hll) noexcept {
  if (is_hll_unpacked(hll)) {
    return hll.size();
  }
  return hll[0] == HLL_PACK_CHAR ? (1 << 8) : (1 << (hll[1] - '0'));
}

Optional<string> f$vk_stats_hll_merge(const array<mixed> &a) noexcept {
  string result;
  char *result_buff = nullptr;
  int result_len = -1;
  for (array<mixed>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (!it.get_value().is_string()) {
      return false;
    }
    string cur = it.get_value().to_string();
    if (result_len == -1) {
      result_len = get_hll_size(cur);
      result.assign((string::size_type)result_len, (char)HLL_FIRST_RANK_CHAR);
      result_buff = result.buffer();
    }
    if (is_hll_unpacked(cur)) {
      if (result_len != cur.size()) {
        return false;
      }
      int i;
      for (i = 0; i < result_len; i++) {
        if (result_buff[i] < cur[i]) {
          result_buff[i] = cur[i];
        }
      }
    } else {
      int i = 1 + (cur[0] == HLL_PACK_CHAR_V2);
      while (i + 2 < cur.size()) {
        int p;
        if (cur[0] == HLL_PACK_CHAR) {
          p = (to_half_byte(cur[i]) << 4) + to_half_byte(cur[i + 1]);
        } else {
          p = (((int)cur[i] - 1) & 0x7f) + (((int)cur[i + 1] - 1) << 7);
        }
        if (p >= result_len) {
          return false;
        }
        if (result_buff[p] < cur[i + 2]) {
          result_buff[p] = cur[i + 2];
        }
        i += 3;
      }
    }
  }
  return result;
}

static int unpack_hll(const string &hll, char *res) noexcept {
  assert(!is_hll_unpacked(hll));
  int m = get_hll_size(hll);
  int pos = 1 + (hll[0] == HLL_PACK_CHAR_V2);
  memset(res, HLL_FIRST_RANK_CHAR, (size_t)m);
  while (pos + 2 < hll.size()) {
    int p;
    if (hll[0] == HLL_PACK_CHAR) {
      p = (to_half_byte(hll[pos]) << 4) + to_half_byte(hll[pos + 1]);
    } else {
      p = (((int)hll[pos] - 1) & 0x7f) + (((int)hll[pos + 1] - 1) << 7);
    }
    if (p >= m) {
      return -1;
    }
    if (res[p] < hll[pos + 2]) {
      res[p] = hll[pos + 2];
    }
    pos += 3;
  }
  if (pos != hll.size()) {
    return -1;
  }
  return m;
}

static Optional<double> hll_count(const string &hll, int m) noexcept {
  double pow_2_32 = (1LL << 32);
  double alpha_m = 0.7213 / (1.0 + 1.079 / m);
  char const *s;

  RuntimeContext::get().static_SB.clean().reserve(HLL_BUF_SIZE);
  char *hll_buf = RuntimeContext::get().static_SB.buffer();

  if (!is_hll_unpacked(hll)) {
    if (unpack_hll(hll, hll_buf) != m) {
      php_warning("Bad HLL string");
      return false;
    }
    s = hll_buf;
  } else {
    s = hll.c_str();
  }
  double c = 0;
  int vz = 0;
  for (int i = 0; i < m; ++i) {
    c += 1.0 / static_cast<double>(1LL << (s[i] - HLL_FIRST_RANK_CHAR));
    vz += (s[i] == HLL_FIRST_RANK_CHAR);
  }
  double e = (alpha_m * m * m) / c;
  if (e <= 5.0 / 2.0 * m && vz > 0) {
    e = 1.0 * m * log(1.0 * m / vz);
  } else {
    if (m == (1 << 8)) {
      if (e > 1.0 / 30.0 * pow_2_32) {
        e = -pow_2_32 * log(1.0 - e / pow_2_32);
      }
    } else if (m == (1 << 14)) {
      if (e < 72000) {
        double bias = 5.9119 * 1.0e-18 * (e * e * e * e) - 1.4253 * 1.0e-12 * (e * e * e) + 1.2940 * 1.0e-7 * (e * e) - 5.2921 * 1.0e-3 * e + 83.3216;
        e -= e * (bias / 100.0);
      }
    } else {
      assert(0);
    }
  }
  return e;
}

/**
 * Do not change implementation of this hash function, because hashes may be saved in a permanent storage.
 * A full copy of the same function exists in vkext-stats.c in vkext.
 */
static long long dl_murmur64a_hash(const void *data, size_t len) noexcept {
  assert((len & 7) == 0);
  unsigned long long m = 0xc6a4a7935bd1e995;
  int r = 47;
  unsigned long long h = 0xcafebabeull ^ (m * len);

  const unsigned char *start = (const unsigned char *)data;
  const unsigned char *end = start + len;

  while (start != end) {
    unsigned long long k = *(unsigned long long *)start;
    k *= m;
    k ^= k >> r;
    k *= m;
    h ^= k;
    h *= m;
    start += 8;
  }

  start = (const unsigned char *)data;

  switch (len & 7) {
    case 7:
      h ^= (unsigned long long)start[6] << 48; /* fallthrough */
    case 6:
      h ^= (unsigned long long)start[5] << 40; /* fallthrough */
    case 5:
      h ^= (unsigned long long)start[4] << 32; /* fallthrough */
    case 4:
      h ^= (unsigned long long)start[3] << 24; /* fallthrough */
    case 3:
      h ^= (unsigned long long)start[2] << 16; /* fallthrough */
    case 2:
      h ^= (unsigned long long)start[1] << 8; /* fallthrough */
    case 1:
      h ^= (unsigned long long)start[0];
      h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

static void hll_add_shifted(unsigned char *hll, int hll_size, long long value) noexcept {
  unsigned long long hash = dl_murmur64a_hash(&(value), sizeof(long long));
  unsigned int idx = hash >> (64LL - hll_size);
  unsigned char rank = (hash == 0) ? 0 : (unsigned char)fmin(__builtin_ctzll(hash) + 1, 64 - hll_size);
  rank += HLL_FIRST_RANK_CHAR;
  if (hll[idx] < rank) {
    hll[idx] = rank;
  }
}

Optional<string> f$vk_stats_hll_add(const string &hll, const array<mixed> &a) noexcept {
  if (!is_hll_unpacked(hll)) {
    return false;
  }
  if (hll.size() != (1 << 8) && hll.size() != (1 << 14)) {
    return false;
  }
  int hll_size = __builtin_ctz(get_hll_size(hll));

  RuntimeContext::get().static_SB.clean().reserve(HLL_BUF_SIZE);
  char *hll_buf = RuntimeContext::get().static_SB.buffer();

  memcpy(hll_buf, hll.c_str(), hll.size());
  for (array<mixed>::const_iterator it = a.begin(); it != a.end(); ++it) {
    hll_add_shifted((unsigned char *)hll_buf, hll_size, it.get_value().to_int());
  }
  return string(hll_buf, hll.size());
}

Optional<string> f$vk_stats_hll_create(const array<mixed> &a, int64_t size) noexcept {
  if (size != (1 << 8) && size != (1 << 14)) {
    return false;
  }
  return f$vk_stats_hll_add(string((string::size_type)size, (char)HLL_FIRST_RANK_CHAR), a);
}

Optional<double> f$vk_stats_hll_count(const string &hll) noexcept {
  int size = get_hll_size(hll);
  if (size == (1 << 8) || size == (1 << 14)) {
    return hll_count(hll, size);
  } else {
    return false;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ copypaste from common/statistics.c
string hll_pack(const string &s, int len) noexcept {
  if (len > MAX_HLL_SIZE || len == 0 || s[0] == HLL_PACK_CHAR || s[0] == HLL_PACK_CHAR_V2) {
    return s;
  }
  unsigned char buf[HLL_BUF_SIZE];
  int p = 0;
  buf[p++] = HLL_PACK_CHAR_V2;
  buf[p++] = (unsigned char)('0' + (unsigned char)(__builtin_ctz(len)));
  assert(__builtin_popcount(len) == 1);
  for (int i = 0; i < len; i++) {
    if (s[i] > HLL_FIRST_RANK_CHAR) {
      if (p + 2 >= len) {
        return s;
      }
      buf[p++] = (unsigned char)((i & 0x7f) + 1);
      buf[p++] = (unsigned char)((i >> 7) + 1);
      buf[p++] = (unsigned char)s[i];
    }
    assert(p < HLL_BUF_SIZE);
  }
  return {(char *)buf, static_cast<string::size_type>(p)};
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optional<string> f$vk_stats_hll_pack(const string &hll) noexcept {
  if (!is_hll_unpacked(hll)) {
    return false;
  }
  return hll_pack(hll, hll.size());
}

Optional<string> f$vk_stats_hll_unpack(const string &hll) noexcept {
  if (is_hll_unpacked(hll)) {
    return false;
  }
  char res[MAX_HLL_SIZE];
  int m = unpack_hll(hll, res);
  if (m == -1) {
    return false;
  }
  return string(res, m);
}

bool f$vk_stats_hll_is_packed(const string &hll) noexcept {
  return !is_hll_unpacked(hll);
}
