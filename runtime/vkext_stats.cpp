#include "runtime/vkext_stats.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#include "degorov/de-archiver.h"

#include "runtime/integer_types.h"

static inline int mymin(int a, int b) {
  return (a < b) ? a : b;
}

#define HLL_FIRST_RANK_CHAR 0x30
#define HLL_PACK_CHAR '!'
#define HLL_PACK_CHAR_V2 '$'
#define TO_HALF_BYTE(c) ((int)(((c > '9') ? (c - 7) : c) - '0'))
#define MAX_HLL_SIZE (1 << 14)
#define HLL_BUF_SIZE (MAX_HLL_SIZE + 1000)

static char hll_buf[HLL_BUF_SIZE];

// Merges std deviations of two sets
// n1, n2 - number of elements
// sum1, sum2 - sums of sets
// nsigma21, nsumna22 - (number of elements) * (std deviation) ^ 2
// return value: (number of elements) * (std deviation) ^ 2 of conjunction
double f$vk_stats_merge_deviation(int n1, Long sum1, double nsigma21, int n2, Long sum2, double nsigma22) {
  if (n1 == 0) {
    if (n2 == 0) {
      return 0.0;
    } else {
      return nsigma22;
    }
  } else {
    if (n2 == 0) {
      return nsigma21;
    }
  }
  double ediff = ((double)sum1.l / n1 - (double)sum2.l / n2);
  return nsigma21 + nsigma22 + (double)n1 / (n1 + n2) * n2 * ediff * ediff;
}

// Recalc std deviations after add
// n - number of elements in set
// sum- sum of sets
// nsigma2 - (number of elements) * (std deviation) ^ 2
// return value: (number of elements) * (std deviation) ^ 2 after add
double f$vk_stats_add_deviation(int n, Long sum, double nsigma2, long long val) {
  if (n == 0) {
    return 0.0;
  }
  double nediff = (double)(sum.l - val * n);
  return nsigma2 + nediff / (n + 1) * nediff / n;
}

//////
//    sample_t fuctions
//////

static unsigned long long seed;

static unsigned int mrand() {
  //http://en.wikipedia.org/wiki/Linear_congruential_generator
  seed = seed * 25214903917ull + 11ull;
  return seed >> 16;
}

static unsigned int rand_exact(unsigned int r) {
  static unsigned int t;
  for (t = mrand(); unlikely(t >= UINT_MAX - (UINT_MAX % r)); t = mrand()) {
  }
  return t % r;
}

static void add_sample(sample_t *sample, int value) {
  static unsigned int tmp;
  if (sample->max_size <= sample->dataset_size) {
    tmp = rand_exact((unsigned int)(sample->dataset_size));
    if (tmp < sample->max_size) {
      sample->values[tmp] = value;
    }
    sample->dataset_size++;
    return;
  }
  sample->values[sample->dataset_size++] = value;
}

static void merge_sample(sample_t *dest, sample_t *sample) {
  if (unlikely(dest->max_size != sample->max_size))
    assert(dest->max_size == sample->max_size);
  int i;
  if (sample->dataset_size <= sample->max_size) {
    for (i = 0; i < sample->dataset_size; i++) {
      add_sample(dest, sample->values[i]);
    }
    return;
  }
  int j;
  int new_dataset_size = dest->dataset_size + sample->dataset_size;
  int dest_size = mymin(dest->dataset_size, dest->max_size);
  for (i = 0, j = 0; i < dest_size; i++) {
    if (rand_exact((unsigned int)new_dataset_size) < dest->dataset_size) {
      dest->values[j++] = dest->values[i];
    }
  }
  for (i = 0; j != dest->max_size; i++) {
    if (rand_exact((unsigned int)(sample->max_size - i)) < dest->max_size - j) {
      dest->values[j++] = sample->values[i];
    }
  }
  assert(j == dest->max_size);
  dest->dataset_size = new_dataset_size;
  //dest->size = dest->max_size;
}

static int get_encoded_sample_max_size(void *buff, int len) {
  char *ptr = (char *)buff;
  int res = int_byte_decode_safe(&ptr, &len);
  return ptr ? res : -1;
}

static int decode_sample(void *buff, int len, sample_t *sample) {
  char *ptr = (char *)buff;
  int max_size = int_byte_decode_safe(&ptr, &len), i;
  if (!ptr) {
    return 0;
  }
  assert (max_size == sample->max_size);
  sample->dataset_size = int_byte_decode_safe(&ptr, &len);
  if (!ptr) {
    return 0;
  }
  int size = mymin(sample->max_size, sample->dataset_size);
  for (i = 0; i < size; i++) {
    sample->values[i] = int_byte_decode_safe(&ptr, &len);
    if (!ptr) {
      return 0;
    }
  }
  if (len) {
    return 0;
  }
  array_int_diff_decode(sample->values + 1, sample->values[0], size - 1);
  return 1;
}

Optional<string> f$vk_stats_decompress_sample(const string &s) {
  int max_size = get_encoded_sample_max_size((void *)s.c_str(), s.size());
  if (max_size < 0) {
    return false;
  }
  int size = offsetof(sample_t, values) + max_size * sizeof(int);
  string result(size, false);
  sample_t *sample = (sample_t *)result.buffer();
  sample->max_size = max_size;
  if (!decode_sample((void *)s.c_str(), s.size(), sample)) {
    return false;
  }
  return result;
}

int check_sample(sample_t *sample, int len) {
  if (len < 4) {
    return 0;
  }
  if (offsetof(sample_t, values) + sample->max_size * sizeof(int) != len) {
    return 0;
  }
  if (sample->dataset_size < 0) {
    return 0;
  }
  return 1;
}

Optional<string> f$vk_stats_merge_samples(const array<var> &a) {
  int max_size = -1;
  for (array<var>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (!it.get_value().is_string()) {
      return false;
    }
    const char *str = it.get_value().to_string().c_str();
    int len = it.get_value().to_string().size();
    if (!check_sample((sample_t *)str, len)) {
      return false;
    }
    if (max_size == -1) {
      max_size = ((const sample_t *)str)->max_size;
    } else if (max_size != ((const sample_t *)str)->max_size) {
      return false;
    }
  }

  sample_t *sample = nullptr;
  string result;

  for (array<var>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (!sample) {
      result = it.get_value().to_string();
      result.make_not_shared();
      sample = (sample_t *)result.c_str();
    } else {
      merge_sample(sample, (sample_t *)it.get_value().to_string().c_str());
    }
  }

  if (!sample) {
    return false;
  }

  return result;
}

Optional<array<int>> f$vk_stats_parse_sample(const string &str) {
  sample_t *s = (sample_t *)str.c_str();
  if (!check_sample(s, str.size())) {
    return false;
  }

  int count = mymin(s->dataset_size, s->max_size);
  array<int> result(array_size(count, 0, true));
  for (int i = 0; i < count; i++) {
    result.set_value(i, s->values[i]);
  }
  return result;
}

static bool is_hll_unpacked(const string &hll) {
  return hll.empty() || (hll[0] != HLL_PACK_CHAR && hll[0] != HLL_PACK_CHAR_V2);
}

static int get_hll_size(const string &hll) {
  if (is_hll_unpacked(hll)) {
    return hll.size();
  }
  return hll[0] == HLL_PACK_CHAR ? (1 << 8) : (1 << (hll[1] - '0'));
}

Optional<string> f$vk_stats_hll_merge(const array<var> &a) {
  string result;
  char *result_buff = 0;
  int result_len = -1;
  for (array<var>::const_iterator it = a.begin(); it != a.end(); ++it) {
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
          p = (TO_HALF_BYTE(cur[i]) << 4) + TO_HALF_BYTE(cur[i + 1]);
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

static int unpack_hll(const string &hll, char *res) {
  assert(!is_hll_unpacked(hll));
  int m = get_hll_size(hll);
  int pos = 1 + (hll[0] == HLL_PACK_CHAR_V2);
  memset(res, HLL_FIRST_RANK_CHAR, (size_t)m);
  while (pos + 2 < hll.size()) {
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
  if (pos != hll.size()) {
    return -1;
  }
  return m;
}


Optional<double> hll_count(const string &hll, int m) {
  double pow_2_32 = (1LL << 32);
  double alpha_m = 0.7213 / (1.0 + 1.079 / m);
  char const *s;
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

extern "C" void hll_add_shifted(unsigned char *a, int hll_size, long long value);

Optional<string> f$vk_stats_hll_add(const string &hll, const array<var> &a) {
  if (!is_hll_unpacked(hll)) {
    return false;
  }
  if (hll.size() != (1 << 8) && hll.size() != (1 << 14)) {
    return false;
  }
  int hll_size = __builtin_ctz(get_hll_size(hll));
  memcpy(hll_buf, hll.c_str(), hll.size());
  for (array<var>::const_iterator it = a.begin(); it != a.end(); ++it) {
    hll_add_shifted((unsigned char *)hll_buf, hll_size, it.get_value().to_int());
  }
  return string(hll_buf, hll.size());
}

Optional<string> f$vk_stats_hll_create(const array<var> &a, int size) {
  if (size != (1 << 8) && size != (1 << 14)) {
    return false;
  }
  return f$vk_stats_hll_add(string((string::size_type)size, (char)HLL_FIRST_RANK_CHAR), a);
}

Optional<double> f$vk_stats_hll_count(const string &hll) {
  int size = get_hll_size(hll);
  if (size == (1 << 8) || size == (1 << 14)) {
    return hll_count(hll, size);
  } else {
    return false;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ copypaste from common/statistics.c
string hll_pack(const string &s, int len) {
  if (len > MAX_HLL_SIZE || len == 0 || s[0] == HLL_PACK_CHAR || s[0] == HLL_PACK_CHAR_V2) {
    return s;
  }
  unsigned char buf[HLL_BUF_SIZE];
  int p = 0;
  buf[p++] = HLL_PACK_CHAR_V2;
  buf[p++] = (unsigned char)('0' + (unsigned char)(__builtin_ctz(len)));
  assert (__builtin_popcount(len) == 1);
  for (int i = 0; i < len; i++) {
    if (s[i] > HLL_FIRST_RANK_CHAR) {
      if (p + 2 >= len) {
        return s;
      }
      buf[p++] = (unsigned char)((i & 0x7f) + 1);
      buf[p++] = (unsigned char)((i >> 7) + 1);
      buf[p++] = (unsigned char)s[i];
    }
    assert (p < HLL_BUF_SIZE);
  }
  return string((char*) buf, p);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optional<string> f$vk_stats_hll_pack(const string &hll) {
  if (!is_hll_unpacked(hll)) {
    return false;
  }
  return hll_pack(hll, hll.size());
}

Optional<string> f$vk_stats_hll_unpack(const string &hll) {
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

bool f$vk_stats_hll_is_packed(const string &hll) {
  return !is_hll_unpacked(hll);
}
