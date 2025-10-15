// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "common/wrappers/likely.h"

#include "vkext/vkext-rpc.h"

#define HLL_FIRST_RANK_CHAR 0x30
#define HLL_PACK_CHAR '!'
#define HLL_PACK_CHAR_V2 '$'
#define TO_HALF_BYTE(c) ((int)(((c > '9') ? (c - 7) : c) - '0'))
#define MAX_HLL_SIZE (1 << 14)
#define HLL_BUF_SIZE (MAX_HLL_SIZE + 1000)

//////
//    hll fuctions
//////

static int is_hll_unpacked(char const *s, VK_LEN_T s_len) {
  return s_len == 0 || (s[0] != HLL_PACK_CHAR && s[0] != HLL_PACK_CHAR_V2);
}

static int get_hll_size(char const *s, VK_LEN_T s_len) {
  if (is_hll_unpacked(s, s_len)) {
    return (int) s_len;
  }
  return s[0] == HLL_PACK_CHAR ? (1 << 8) : (1 << (s[1] - '0'));
}

PHP_FUNCTION (vk_stats_hll_merge) {
  zval *z;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &z) == FAILURE) {
    return;
  }
  if (Z_TYPE_P (z) != IS_ARRAY) {
    RETURN_FALSE;
    return;
  }
  VK_ZVAL_API_P zkey;
  char *result = 0;
  int result_len = -1;
  VK_ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z), zkey) {
    if (VK_Z_API_TYPE(zkey) != IS_STRING) {
      if (result)
        efree(result);
      RETURN_FALSE;
      return;
    }
    if (result_len == -1) {
      result_len = get_hll_size(Z_STRVAL_P(VK_ZVAL_API_TO_ZVALP(zkey)), Z_STRLEN_P(VK_ZVAL_API_TO_ZVALP(zkey)));
      result = static_cast<char *>(emalloc((size_t)result_len + 1));
      int i;
      for (i = 0; i < result_len; i++) {
        result[i] = '0';
      }
      result[result_len] = 0;
    }
    if (is_hll_unpacked(Z_STRVAL_P(VK_ZVAL_API_TO_ZVALP(zkey)), Z_STRLEN_P(VK_ZVAL_API_TO_ZVALP(zkey)))) {
      if (static_cast<size_t>(result_len) != Z_STRLEN_P(VK_ZVAL_API_TO_ZVALP(zkey))) {
        if (result)
          efree(result);
        RETURN_FALSE;
        return;
      }
      int i;
      for (i = 0; i < result_len; i++) {
        if (result[i] < Z_STRVAL_P(VK_ZVAL_API_TO_ZVALP(zkey))[i]) {
          result[i] = Z_STRVAL_P(VK_ZVAL_API_TO_ZVALP(zkey))[i];
        }
      }
    } else {
      char *cur = Z_STRVAL_P(VK_ZVAL_API_TO_ZVALP(zkey));
      int cur_len = Z_STRLEN_P(VK_ZVAL_API_TO_ZVALP(zkey));
      int i = 1 + (cur[0] == HLL_PACK_CHAR_V2);
      while (i + 2 < cur_len) {
        int p;
        if (cur[0] == HLL_PACK_CHAR) {
          p = (TO_HALF_BYTE(cur[i]) << 4) + TO_HALF_BYTE(cur[i + 1]);
        } else {
          p = (((int)cur[i] - 1) & 0x7f) + (((int)cur[i + 1] - 1) << 7);
        }
        if (p >= result_len) {
          if (result)
            efree(result);
          RETURN_FALSE;
          return;
        }
        if (result[p] < cur[i + 2]) {
          result[p] = cur[i + 2];
        }
        i += 3;
      }
    }
  } VK_ZEND_HASH_FOREACH_END();
  VK_RETURN_STRINGL_NOD(result, result_len);
}

static int unpack_hll(char const *hll, VK_LEN_T hll_size, char *res) {
  assert(!is_hll_unpacked(hll, hll_size));
  int m = get_hll_size(hll, hll_size);
  int pos = 1 + (hll[0] == HLL_PACK_CHAR_V2);
  memset(res, HLL_FIRST_RANK_CHAR, (size_t)m);
  while (static_cast<VK_LEN_T>(pos + 2) < hll_size) {
    int p;
    if (hll[0] == HLL_PACK_CHAR) {
      p = (TO_HALF_BYTE(hll[pos]) << 4) + TO_HALF_BYTE(hll[pos + 1]);
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
  if (static_cast<VK_LEN_T>(pos) != hll_size) {
    return -1;
  }
  return m;
}

void hll_pack (const unsigned char *s, VK_LEN_T len, unsigned char **res_s, VK_LEN_T *res_len) {
  if (len > MAX_HLL_SIZE || len == 0 || s[0] == HLL_PACK_CHAR || s[0] == HLL_PACK_CHAR_V2) {
    *res_len = len;
    *res_s = static_cast<unsigned char *>(emalloc (sizeof (unsigned char *) * len));
    memcpy (*res_s, s, (size_t) *res_len);
    return;
  }

  unsigned char *buf = static_cast<unsigned char *>(emalloc (sizeof (unsigned char *) * HLL_BUF_SIZE));

  VK_LEN_T p = 0, i;
  buf[p++] = HLL_PACK_CHAR_V2;
  buf[p++] = (unsigned  char) ('0' + (unsigned  char) (__builtin_ctz (len)));
  assert (__builtin_popcount (len) == 1);
  for (i = 0; i < len; i++) {
    if (s[i] > HLL_FIRST_RANK_CHAR) {
      if (p + 2 >= len) {
        *res_len = len;
        *res_s = static_cast<unsigned char *>(emalloc (sizeof (unsigned char *) * len));
        memcpy (*res_s, s, (size_t) *res_len);
        efree (buf);
        return;
      }
      buf[p++] = (unsigned char) ((i & 0x7f) + 1);
      buf[p++] = (unsigned char) ((i >> 7) + 1);
      buf[p++] = s[i];
    }
    assert (p < HLL_BUF_SIZE);
  }

  *res_len = p;
  *res_s = static_cast<unsigned char *>(emalloc (sizeof (unsigned char) * *res_len));
  memcpy (*res_s, buf, (size_t) *res_len);

  efree (buf);
}

double hll_count(char const *hll_table, int hll_table_size, int m) {
  int i;
  double pow_2_32 = (1LL << 32);
  double alpha_m = 0.7213 / (1.0 + 1.079 / m);
  static char hll_buf[HLL_BUF_SIZE];
  char const *s;
  if (!is_hll_unpacked(hll_table, hll_table_size)) {
    unpack_hll(hll_table, hll_table_size, hll_buf);
    s = hll_buf;
  } else {
    s = hll_table;
  }
  double c = 0;
  int vz = 0;
  for (i = 0; i < m; ++i) {
    c += 1.0 / (1LL << (s[i] - HLL_FIRST_RANK_CHAR));
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
        double bias = 5.9119 * 1.0e-18 * (e * e * e * e)
                      - 1.4253 * 1.0e-12 * (e * e * e) +
                      1.2940 * 1.0e-7 * (e * e)
                      - 5.2921 * 1.0e-3 * e +
                      83.3216;
        e -= e * (bias / 100.0);
      }
    } else {
      assert(0);
    }
  }
  return e;
}

PHP_FUNCTION (vk_stats_hll_count) {
  char *s;
  VK_LEN_T s_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &s_len) == FAILURE) {
    return;
  }
  int len = get_hll_size(s, s_len);

  if (len == (1 << 8) || len == (1 << 14)) {
    RETURN_DOUBLE(hll_count(s, s_len, len));
  } else {
    RETURN_FALSE;
  }
}

/**
 * Do not change implementation of this hash function, because hashes may be saved in a permanent storage.
 * A full copy of the same function exists in vkext-stats.cpp in KPHP.
 */
long long dl_murmur64a_hash(const void *data, size_t len) {
  assert ((len & 7) == 0);
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

  switch(len & 7) {
    case 7: h ^= (unsigned long long)start[6] << 48;
    /* fallthrough */
    case 6: h ^= (unsigned long long)start[5] << 40;
    /* fallthrough */
    case 5: h ^= (unsigned long long)start[4] << 32;
    /* fallthrough */
    case 4: h ^= (unsigned long long)start[3] << 24;
    /* fallthrough */
    case 3: h ^= (unsigned long long)start[2] << 16;
    /* fallthrough */
    case 2: h ^= (unsigned long long)start[1] << 8;
    /* fallthrough */
    case 1: h ^= (unsigned long long)start[0];
      h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

static int hll_add(char *hll, int size, zval *z) {
  if (z == NULL) {
    return 1;
  }
  VK_ZVAL_API_P zkey;
  int hll_size = __builtin_ctz(size);
  VK_ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z), zkey) {
    // like hll_add_shifted() in KPHP
    long long value = parse_zend_long(zkey);
    unsigned long long hash = dl_murmur64a_hash(&(value), sizeof(long long));
    unsigned int idx = hash >> (64LL - hll_size);
    unsigned char rank = (hash == 0) ? 0 : (unsigned char)fmin(__builtin_ctzll(hash) + 1, 64 - hll_size);
    rank += HLL_FIRST_RANK_CHAR;
    if (hll[idx] < rank) {
      hll[idx] = rank;
    }
  } VK_ZEND_HASH_FOREACH_END();
  return 1;
}

PHP_FUNCTION (vk_stats_hll_create) {
  zval *z = NULL;
  long size = 256;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|al", &z, &size) == FAILURE) {
    return;
  }
  if (z != NULL && Z_TYPE_P (z) != IS_ARRAY) {
    RETURN_FALSE;
    return;
  }
  if (size != (1 << 8) && size != (1 << 14)) {
    RETURN_FALSE;
  }
  char *result = static_cast<char *>(emalloc((size_t)(size + 1)));
  int i;
  for (i = 0; i < size; i++) {
    result[i] = HLL_FIRST_RANK_CHAR;
  }
  result[size] = 0;
  if (!hll_add(result, size, z)) {
    efree(result);
    RETURN_FALSE;
    return;
  }
  VK_RETURN_STRINGL_NOD(result, size);
}

PHP_FUNCTION (vk_stats_hll_add) {
  char *hll;
  zval *z;
  VK_LEN_T hll_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa", &hll, &hll_len, &z) == FAILURE) {
    return;
  }
  if (Z_TYPE_P (z) != IS_ARRAY || !is_hll_unpacked(hll, hll_len) || (hll_len != (1 << 8) && hll_len != (1 << 14))) {
    RETURN_FALSE;
    return;
  }
  char *result = static_cast<char *>(emalloc((size_t)(hll_len + 1)));
  for (VK_LEN_T i = 0; i <= hll_len; i++) {
    result[i] = hll[i];
  }
  if (!hll_add(result, hll_len, z)) {
    efree(result);
    RETURN_FALSE;
    return;
  }
  VK_RETURN_STRINGL_NOD(result, hll_len);
}

PHP_FUNCTION (vk_stats_hll_pack) {
  char *s;
  VK_LEN_T s_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &s_len) == FAILURE) {
    return;
  }
  if (!is_hll_unpacked(s, s_len)) {
    RETURN_FALSE;
  }
  char* res;
  VK_LEN_T res_len;
  hll_pack((unsigned char *) s, s_len, (unsigned char **) &res, &res_len);
  VK_RETURN_STRINGL_NOD(res, res_len);
}

PHP_FUNCTION (vk_stats_hll_unpack) {
  char *s;
  VK_LEN_T s_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &s_len) == FAILURE) {
    return;
  }
  if (is_hll_unpacked(s, s_len)) {
    RETURN_FALSE;
  }
  char* res = static_cast<char *>(emalloc(MAX_HLL_SIZE));
  int m = unpack_hll(s, s_len, res);
  if (m == -1) {
    RETURN_FALSE;
  }
  assert(m >= 0);
  VK_LEN_T res_len = (VK_LEN_T) m;
  VK_RETURN_STRINGL_NOD(res, res_len);
}

PHP_FUNCTION (vk_stats_hll_is_packed) {
  char *s;
  VK_LEN_T s_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &s, &s_len) == FAILURE) {
    return;
  }
  RETURN_BOOL(!is_hll_unpacked(s, s_len));
}
