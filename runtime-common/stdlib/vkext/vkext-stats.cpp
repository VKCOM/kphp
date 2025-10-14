// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/vkext/vkext-stats.h"

#include <climits>
#include <cstring>

namespace {

constexpr auto HLL_FIRST_RANK_CHAR = 0x30;
constexpr auto HLL_PACK_CHAR = '!';
constexpr auto HLL_PACK_CHAR_V2 = '$';
constexpr auto MAX_HLL_SIZE = (1 << 14);
constexpr auto HLL_BUF_SIZE = (MAX_HLL_SIZE + 1000);

auto to_half_byte(char c) {
  return (((c > '9') ? (c - 7) : c) - '0');
}

//////
//    hll fuctions
//////

bool is_hll_unpacked(const string& hll) noexcept {
  return hll.empty() || (hll[0] != HLL_PACK_CHAR && hll[0] != HLL_PACK_CHAR_V2);
}

int get_hll_size(const string& hll) noexcept {
  if (is_hll_unpacked(hll)) {
    return hll.size();
  }
  return hll[0] == HLL_PACK_CHAR ? (1 << 8) : (1 << (hll[1] - '0'));
}

int unpack_hll(const string& hll, char* res) noexcept {
  php_assert(!is_hll_unpacked(hll));
  int m = get_hll_size(hll);
  int pos = 1 + (hll[0] == HLL_PACK_CHAR_V2);
  memset(res, HLL_FIRST_RANK_CHAR, m);
  while (pos + 2 < hll.size()) {
    int p;
    if (hll[0] == HLL_PACK_CHAR) {
      p = (to_half_byte(hll[pos]) << 4) + to_half_byte(hll[pos + 1]);
    } else {
      p = ((hll[pos] - 1) & 0x7f) + ((hll[pos + 1] - 1) << 7);
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

Optional<double> hll_count(const string& hll, int m) noexcept {
  char hll_buf[HLL_BUF_SIZE];

  double pow_2_32 = (1LL << 32);
  double alpha_m = 0.7213 / (1.0 + 1.079 / m);
  char const* s;
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
      php_assert(0);
    }
  }
  return e;
}

/**
 * Do not change implementation of this hash function, because hashes may be saved in a permanent storage.
 * A full copy of the same function exists in vkext-stats.c in vkext.
 */
long long dl_murmur64a_hash(const void* data, size_t len) noexcept {
  php_assert((len & 7) == 0);
  unsigned long long m = 0xc6a4a7935bd1e995;
  int r = 47;
  unsigned long long h = 0xcafebabeull ^ (m * len);

  const unsigned char* start = static_cast<const unsigned char*>(data);
  const unsigned char* end = start + len;

  while (start != end) {
    unsigned long long k = *reinterpret_cast<const unsigned long long*>(start);
    k *= m;
    k ^= k >> r;
    k *= m;
    h ^= k;
    h *= m;
    start += 8;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

void hll_add_shifted(unsigned char* hll, int hll_size, long long value) noexcept {
  unsigned long long hash = dl_murmur64a_hash(&(value), sizeof(long long));
  unsigned int idx = hash >> (64LL - hll_size);
  unsigned char rank = (hash == 0) ? 0 : static_cast<unsigned char>(fmin(__builtin_ctzll(hash) + 1, 64 - hll_size));
  rank += HLL_FIRST_RANK_CHAR;
  if (hll[idx] < rank) {
    hll[idx] = rank;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ copypaste from common/statistics.c
string hll_pack(const string& s, int len) noexcept {
  if (len > MAX_HLL_SIZE || len == 0 || s[0] == HLL_PACK_CHAR || s[0] == HLL_PACK_CHAR_V2) {
    return s;
  }
  unsigned char buf[HLL_BUF_SIZE];
  int p = 0;
  buf[p++] = HLL_PACK_CHAR_V2;
  buf[p++] = '0' + __builtin_ctz(len);
  php_assert(__builtin_popcount(len) == 1);
  for (int i = 0; i < len; i++) {
    if (s[i] > HLL_FIRST_RANK_CHAR) {
      if (p + 2 >= len) {
        return s;
      }
      buf[p++] = static_cast<unsigned char>((i & 0x7f) + 1);
      buf[p++] = (i >> 7) + 1;
      buf[p++] = s[i];
    }
    php_assert(p < HLL_BUF_SIZE);
  }
  return {reinterpret_cast<char*>(buf), static_cast<string::size_type>(p)};
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}

Optional<string> f$vk_stats_hll_merge(const array<mixed>& a) noexcept {
  string result;
  char* result_buff = nullptr;
  int result_len = -1;
  for (array<mixed>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (!it.get_value().is_string()) {
      return false;
    }
    string cur = it.get_value().to_string();
    if (result_len == -1) {
      result_len = get_hll_size(cur);
      result.assign(result_len, static_cast<char>(HLL_FIRST_RANK_CHAR));
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
          p = ((cur[i] - 1) & 0x7f) + ((cur[i + 1] - 1) << 7);
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

Optional<string> f$vk_stats_hll_add(const string& hll, const array<mixed>& a) noexcept {
  auto res = string(HLL_BUF_SIZE, false);
  auto hll_buf = res.buffer();

  if (!is_hll_unpacked(hll)) {
    return false;
  }
  if (hll.size() != (1 << 8) && hll.size() != (1 << 14)) {
    return false;
  }
  int hll_size = __builtin_ctz(get_hll_size(hll));
  memcpy(hll_buf, hll.c_str(), hll.size());
  for (array<mixed>::const_iterator it = a.begin(); it != a.end(); ++it) {
    hll_add_shifted(reinterpret_cast<unsigned char*>(hll_buf), hll_size, it.get_value().to_int());
  }

  res.shrink(hll.size());
  return res;
}

Optional<string> f$vk_stats_hll_create(const array<mixed>& a, int64_t size) noexcept {
  if (size != (1 << 8) && size != (1 << 14)) {
    return false;
  }
  return f$vk_stats_hll_add(string(size, static_cast<char>(HLL_FIRST_RANK_CHAR)), a);
}

Optional<double> f$vk_stats_hll_count(const string& hll) noexcept {
  int size = get_hll_size(hll);
  if (size == (1 << 8) || size == (1 << 14)) {
    return hll_count(hll, size);
  } else {
    return false;
  }
}

Optional<string> f$vk_stats_hll_pack(const string& hll) noexcept {
  if (!is_hll_unpacked(hll)) {
    return false;
  }
  return hll_pack(hll, hll.size());
}

Optional<string> f$vk_stats_hll_unpack(const string& hll) noexcept {
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

bool f$vk_stats_hll_is_packed(const string& hll) noexcept {
  return !is_hll_unpacked(hll);
}
