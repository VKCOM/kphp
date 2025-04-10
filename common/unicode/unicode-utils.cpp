// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/unicode/unicode-utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "auto/common/unicode-utils-auto.h"

#include "common/unicode/utf8-utils.h"

/* Search generated ranges for specified character */
static int binary_search_ranges(const int* ranges, int r, int code) {
  if ((unsigned int)code > 0x10ffff) {
    return 0;
  }

  int l = 0;
  while (l < r) {
    int m = ((l + r + 2) >> 2) << 1;
    if (ranges[m] <= code) {
      l = m;
    } else {
      r = m - 2;
    }
  }

  int t = ranges[l + 1];
  if (t < 0) {
    return code - ranges[l] + (~t);
  }
  if (t <= 0x10ffff) {
    return t;
  }
  switch (t - 0x200000) {
  case 0:
    return (code & -2);
  case 1:
    return (code | 1);
  case 2:
    return ((code - 1) | 1);
  default:
    assert(0);
    exit(1);
  }
}

/* Convert character to upper case */
int unicode_toupper(int code) {
  if ((unsigned int)code < (unsigned int)TABLE_SIZE) {
    return to_upper_table[code];
  } else {
    return binary_search_ranges(to_upper_table_ranges, to_upper_table_ranges_size, code);
  }
}

/* Convert character to lower case */
int unicode_tolower(int code) {
  if ((unsigned int)code < (unsigned int)TABLE_SIZE) {
    return to_lower_table[code];
  } else {
    return binary_search_ranges(to_lower_table_ranges, to_lower_table_ranges_size, code);
  }
}

/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
int prepare_search_string(int* input) {
  int i;
  int* output = input;
  for (i = 0; input[i]; i++) {
    int c = input[i], new_c;
    if ((unsigned int)c < (unsigned int)TABLE_SIZE) {
      new_c = prepare_table[c];
    } else {
      new_c = binary_search_ranges(prepare_table_ranges, prepare_table_ranges_size, c);
    }
    if (new_c) {
      if (new_c != 0x20 || (output > input && output[-1] != 0x20)) {
        *output++ = new_c;
      }
    }
  }
  if (output > input && output[-1] == 0x20) {
    output--;
  }
  *output = 0;
  return output - input;
}

#define MAX_NAME_SIZE 65536
static char prep_buf[4 * MAX_NAME_SIZE + 4];
int prep_ibuf[MAX_NAME_SIZE + 4];
static int prep_ibuf_res[MAX_NAME_SIZE + 4];
static int* words_ibuf[MAX_NAME_SIZE + 4];

int stricmp_void(const void* x, const void* y) {
  const int* s1 = *(const int**)x;
  const int* s2 = *(const int**)y;
  while (*s1 == *s2 && *s1 != ' ')
    s1++, s2++;
  return *s1 - *s2;
}

int* prepare_str_unicode(const int* x) {
  int* v = prep_ibuf;

  int n;
  if (v != x) {
    for (n = 0; x[n]; n++) {
      v[n] = x[n];
    }
    v[n] = 0;
  }

  n = prepare_search_string(v);
  v[n] = ' ';

  int i = 0, k = 0;
  while (i < n) {
    words_ibuf[k++] = v + i;
    while (v[i] && v[i] != ' ') {
      i++;
    }
    i++;
  }

  qsort(words_ibuf, (size_t)k, sizeof(int*), stricmp_void);

  int j = 0;
  for (i = 0; i < k; i++) {
    if (j == 0 || stricmp_void(&words_ibuf[j - 1], &words_ibuf[i])) {
      words_ibuf[j++] = words_ibuf[i];
    } else {
      words_ibuf[j - 1] = words_ibuf[i];
    }
  }
  k = j;

  int* res = prep_ibuf_res;
  for (i = 0; i < k; i++) {
    int* tmp = words_ibuf[i];
    while (*tmp != ' ') {
      *res++ = *tmp++;
    }
    *res++ = '+';
  }
  *res++ = 0;

  assert(res - prep_ibuf_res < MAX_NAME_SIZE);
  return prep_ibuf_res;
}

const char* clean_str_unicode(const int* xx) {
  assert(xx != NULL);

  int* v = prepare_str_unicode(xx);
  int l = put_string_utf8(v, prep_buf);
  assert(l < sizeof(prep_buf));

  char *s = prep_buf, *x = prep_buf;
  int skip;

  while (*x != 0) {
    skip = !strncmp(x, "amp+", 4) || !strncmp(x, "gt+", 3) || !strncmp(x, "lt+", 3) || !strncmp(x, "quot+", 5) || !strncmp(x, "ft+", 3) ||
           !strncmp(x, "feat+", 5) ||
           (((x[0] == '1' && x[1] == '9') || (x[0] == '2' && x[1] == '0')) && ('0' <= x[2] && x[2] <= '9') && ('0' <= x[3] && x[3] <= '9') && x[4] == '+') ||
           !strncmp(x, "092+", 4) || !strncmp(x, "33+", 3) || !strncmp(x, "34+", 3) || !strncmp(x, "36+", 3) || !strncmp(x, "39+", 3) ||
           !strncmp(x, "60+", 3) || !strncmp(x, "62+", 3) || !strncmp(x, "8232+", 5) || !strncmp(x, "8233+", 5);
    do {
      *s = *x;
      if (!skip) {
        s++;
      }
    } while (*x++ != '+');
  }
  *s = 0;

  return prep_buf;
}

const char* clean_str(const char* x) {
  if (x == NULL || strlen(x) >= MAX_NAME_SIZE) {
    return x;
  }

  html_string_to_utf8(x, prep_ibuf);
  return clean_str_unicode(prep_ibuf);
}
