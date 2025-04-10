// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

void string_to_utf8(const char* s, int* v);
void string_to_utf8_len(const char* s, int s_len, int* v);
void html_string_to_utf8(const char* s, int* v);

int put_string_utf8(const int* v, char* s); // returns len of written string

int simplify_character(int c);

int translit_string_utf8_from_en_to_ru(int* input, int* output);
int translit_string_utf8_from_ru_to_en(int* input, int* output);
int convert_language(int x);
int has_bad_symbols(int* v_s);

static inline int get_char_utf8(int* x, const char* s) {
#define CHECK(condition)                                                                                                                                       \
  if (!(condition)) {                                                                                                                                          \
    *x = -1;                                                                                                                                                   \
    return -1;                                                                                                                                                 \
  }
  unsigned int a = (unsigned char)s[0];
  if ((a & 0x80) == 0) {
    *x = a;
    return (a != 0);
  }

  CHECK((a & 0x40) != 0);

  unsigned int b = (unsigned char)s[1];
  CHECK((b & 0xc0) == 0x80);
  if ((a & 0x20) == 0) {
    CHECK((a & 0x1e) > 0);
    *x = (((a & 0x1f) << 6) | (b & 0x3f));
    return 2;
  }

  unsigned int c = (unsigned char)s[2];
  CHECK((c & 0xc0) == 0x80);
  if ((a & 0x10) == 0) {
    CHECK(((a & 0x0f) | (b & 0x20)) > 0);
    *x = (((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f));
    return 3;
  }

  unsigned int d = (unsigned char)s[3];
  CHECK((d & 0xc0) == 0x80);
  if ((a & 0x08) == 0) {
    CHECK(((a & 0x07) | (b & 0x30)) > 0);
    *x = (((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f));
    return 4;
  }

  unsigned int e = (unsigned char)s[4];
  CHECK((e & 0xc0) == 0x80);
  if ((a & 0x04) == 0) {
    CHECK(((a & 0x03) | (b & 0x38)) > 0);
    *x = (((a & 0x03) << 24) | ((b & 0x3f) << 18) | ((c & 0x3f) << 12) | ((d & 0x3f) << 6) | (e & 0x3f));
    return 5;
  }

  unsigned int f = (unsigned char)s[5];
  CHECK((f & 0xc0) == 0x80);
  if ((a & 0x02) == 0) {
    CHECK(((a & 0x01) | (b & 0x3c)) > 0);
    *x = (((a & 0x01) << 30) | ((b & 0x3f) << 24) | ((c & 0x3f) << 18) | ((d & 0x3f) << 12) | ((e & 0x3f) << 6) | (f & 0x3f));
    return 6;
  }

  CHECK(0);
#undef CHECK
}

static inline int put_char_utf8(unsigned int x, char* s) {
  if (x <= 0x7f) {
    s[0] = (char)x;
    return 1;
  } else if (x <= 0x7ff) {
    s[0] = (char)(((x >> 6) | 0xc0) & 0xdf);
    s[1] = (char)(((x) | 0x80) & 0xbf);
    return 2;
  } else if (x <= 0xffff) {
    s[0] = (char)(((x >> 12) | 0xe0) & 0xef);
    s[1] = (char)(((x >> 6) | 0x80) & 0xbf);
    s[2] = (char)(((x) | 0x80) & 0xbf);
    return 3;
  } else if (x <= 0x1fffff) {
    s[0] = (char)(((x >> 18) | 0xf0) & 0xf7);
    s[1] = (char)(((x >> 12) | 0x80) & 0xbf);
    s[2] = (char)(((x >> 6) | 0x80) & 0xbf);
    s[3] = (char)(((x) | 0x80) & 0xbf);
    return 4;
  } else {
    // ASSERT(0, "bad output");
  }
  return 0;
}

// returns true, if the given value is not the first code unit of a UTF sequence
constexpr bool is_invalid_utf8_first_byte(char c) {
  return (c & 0xc0) == 0x80;
}
