#include "runtime/vkext.h"

#include <sys/time.h>

#include "auto/flex/vkext_flex_auto.h"
#include "common/string-processing.h"

#include "runtime/misc.h"

static int utf8_to_win_convert_0x400[256] = {-1, 0xa8, 0x80, 0x81, 0xaa, 0xbd, 0xb2, 0xaf, 0xa3, 0x8a, 0x8c, 0x8e, 0x8d, -1, 0xa1, 0x8f, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, -1, 0xb8, 0x90, 0x83, 0xba, 0xbe, 0xb3, 0xbf, 0xbc, 0x9a, 0x9c, 0x9e, 0x9d, -1, 0xa2, 0x9f, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xa5, 0xb4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int utf8_to_win_convert_0x2000[256] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x96, 0x97, -1, -1, -1, 0x91, 0x92, 0x82, -1, 0x93, 0x94, 0x84, -1, 0x86, 0x87, 0x95, -1, -1, -1, 0x85, -1, -1, -1, -1, 0xda, 0xda, -1, 0xda, -1, 0x89, -1, -1, -1, -1, -1, -1, -1, -1, 0x8b, 0x9b, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x88, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int utf8_to_win_convert_0xff00[256] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x20, -1, -1};
static int utf8_to_win_convert_0x2100[256] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xb9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int utf8_to_win_convert_0x000[256] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xa0, -1, -1, -1, 0xa4, -1, 0xa6, 0xa7, -1, 0xa9, -1, 0xab, 0xac, 0xad, 0xae, -1, 0xb0, 0xb1, -1, -1, -1, 0xb5, 0xb6, 0xb7, -1, -1, -1, 0xbb, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int win_to_utf8_convert[256] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x402, 0x403, 0x201a, 0x453, 0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x409, 0x2039, 0x40a, 0x40c, 0x40b, 0x40f, 0x452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x0, 0x2122, 0x459, 0x203a, 0x45a, 0x45c, 0x45b, 0x45f, 0xa0, 0x40e, 0x45e, 0x408, 0xa4, 0x490, 0xa6, 0xa7, 0x401, 0xa9, 0x404, 0xab, 0xac, 0xad, 0xae, 0x407, 0xb0, 0xb1, 0x406, 0x456, 0x491, 0xb5, 0xb6, 0xb7, 0x451, 0x2116, 0x454, 0xbb, 0x458, 0x405, 0x455, 0x457, 0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41a, 0x41b, 0x41c, 0x41d, 0x41e, 0x41f, 0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42a, 0x42b, 0x42c, 0x42d, 0x42e, 0x42f, 0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e, 0x43f, 0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449, 0x44a, 0x44b, 0x44c, 0x44d, 0x44e, 0x44f};

inline int utf8_to_win_char (int c) {
  if (c < 0x80) {
    return c;
  }
  switch (c & ~0xff) {
    case 0x0400:
      return utf8_to_win_convert_0x400[c & 0xff];
    case 0x2000:
      return utf8_to_win_convert_0x2000[c & 0xff];
    case 0xff00:
      return utf8_to_win_convert_0xff00[c & 0xff];
    case 0x2100:
      return utf8_to_win_convert_0x2100[c & 0xff];
    case 0x0000:
      return utf8_to_win_convert_0x000[c & 0xff];
  }
  return -1;
}

#define BUFF_LEN (1 << 16)
static char buff[BUFF_LEN];
char *wptr;

char *result_buff;
int result_buff_len;
int result_buff_pos;
#define cur_buff_len (int)((wptr - buff) + result_buff_pos)

inline void init_buff (void) {
  wptr = buff;
  result_buff = NULL;
  result_buff_len = 0;
  result_buff_pos = 0;
}

inline void free_buff (void) {
  if (result_buff_len) {
    dl::deallocate (result_buff, result_buff_len);
  }
}

inline void realloc_buff (void) {
  if (!result_buff_len) {
    result_buff_len = 2 * BUFF_LEN;
    result_buff = (char *)dl::allocate (result_buff_len);
  } else {
    php_assert (result_buff_len < (1 << 30));
    result_buff = (char *)dl::reallocate (result_buff, 2 * result_buff_len, result_buff_len);
    result_buff_len *= 2;
  }
}

inline void flush_buff (void) {
  while (cur_buff_len > result_buff_len) {
    realloc_buff();
  }
  memcpy (result_buff + result_buff_pos, buff, wptr - buff);
  result_buff_pos += (int)(wptr - buff);
  wptr = buff;
}

inline string finish_buff (int max_len) {
  int len = cur_buff_len;
  if (max_len && max_len < len) {
    len = max_len;
  }

  if (result_buff_len) {
    flush_buff();
    string res (result_buff, len);
    free_buff();
    return res;
  }

  return string (buff, len);
}

inline void write_buff (const char *s, int l) {
  while (l > 0) {
    if (unlikely (wptr == buff + BUFF_LEN)) {
      flush_buff();
    }
    int ll = min (l, (int)(buff + BUFF_LEN - wptr));
    memcpy (wptr, s, ll);
    wptr += ll;
    s += ll;
    l -= ll;
  }
}

inline void write_buff_set_pos (int pos) {
  if (pos > cur_buff_len) {
    return;
  }
  if (pos >= result_buff_pos) {
    wptr = (pos - result_buff_pos) + buff;
    return;
  }
  result_buff_pos = pos;
  wptr = buff;
}

inline void write_buff_char_pos (char c, int pos) {
  if (pos > cur_buff_len) {
    return;
  }
  if (pos >= result_buff_pos) {
    *((pos - result_buff_pos) + buff) = c;
    return;
  }
  *(result_buff + pos) = c;
}


inline void write_buff_char (char c) {
  if (unlikely (wptr == buff + BUFF_LEN)) {
    flush_buff();
  }
  *wptr++ = c;
}

inline void write_buff_char_2 (char c1, char c2) {
  if (unlikely (wptr >= buff + BUFF_LEN - 1)) {
    flush_buff();
  }
  *wptr++ = c1;
  *wptr++ = c2;
}

inline void write_buff_char_3 (char c1, char c2, char c3) {
  if (unlikely (wptr >= buff + BUFF_LEN - 2)) {
    flush_buff();
  }
  *wptr++ = c1;
  *wptr++ = c2;
  *wptr++ = c3;
}

inline void write_buff_char_4 (char c1, char c2, char c3, char c4) {
  if (unlikely (wptr >= buff + BUFF_LEN - 3)) {
    flush_buff();
  }
  *wptr++ = c1;
  *wptr++ = c2;
  *wptr++ = c3;
  *wptr++ = c4;
}

inline void write_buff_int (int x) {
  if (unlikely (wptr + 25 > buff + BUFF_LEN)) {
    flush_buff();
  }
  wptr += sprintf (wptr, "%d", x);
}


int utf8_to_win (const char *s, int len, int max_len, bool exit_on_error) {
  int st = 0;
  int acc = 0;
  int i;
//  if (max_len && len > 3 * max_len) {
//    len = 3 * max_len;
//  }
  for (i = 0; i < len; i++) {
    if (max_len && cur_buff_len >= max_len) {
      break;
    }
    int c = (unsigned char)s[i];
    if (c < 0x80) {
      if (st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff ("?1?", 3);
      }
      write_buff_char ((char)c);
      st = 0;
    } else if ((c & 0xc0) == 0x80) {
      if (!st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff ("?2?", 3);
        continue;
      }
      acc <<= 6;
      acc += c - 0x80;
      if (!--st) {
        if (acc < 0x80) {
          if (exit_on_error) {
            return -1;
          }
          write_buff ("?3?", 3);
        } else {
          int d = utf8_to_win_char (acc);
          if (d != -1 && d) {
            write_buff_char ((char)d);
          } else {
            write_buff_char_2 ('&', '#');
            write_buff_int (acc);
            write_buff_char (';');
          }
        }
      }
    } else { // if ((c & 0xc0) == 0xc0)
      if (st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff ("?4?", 3);
      }
      c -= 0xc0;
      st = 0;
      if (c < 32) {
        acc = c;
        st = 1;
      } else if (c < 48) {
        acc = c - 32;
        st = 2;
      } else if (c < 56) {
        acc = c - 48;
        st = 3;
      } else {
        if (exit_on_error) {
          return -1;
        }
        write_buff ("?5?", 3);
      }
    }
  }
  if (st) {
    if (exit_on_error) {
      return -1;
    }
    write_buff ("?6?", 3);
  }
  return 1;
}

void write_char_utf8 (int c) {
  if (!c) {
    return;
  }
  if (c < 128) {
    write_buff_char ((char)c);
    return;
  }
  if (c < 0x800) {
    write_buff_char_2 ((char)(0xc0 + (c >> 6)), (char)(0x80 + (c & 63)));
    return;
  }
  if (c < 0x10000) {
    write_buff_char_3 ((char)(0xe0 + (c >> 12)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }
  if (c >= 0x1f000 && c <= 0x1f9ff) {
    write_buff_char_4 ((char)(0xf0 + (c >> 18)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }
  write_buff_char_2 ('$', '#');
  write_buff_int (c);
  write_buff_char (';');
}

void write_char_utf8_no_escape (int c) {
  if (!c) {
    return;
  }
  if (c < 128) {
    write_buff_char ((char)c);
    return;
  }
  if (c < 0x800) {
    write_buff_char_2 ((char)(0xc0 + (c >> 6)), (char)(0x80 + (c & 63)));
    return;
  }
  if (c < 0x10000) {
    write_buff_char_3 ((char)(0xe0 + (c >> 12)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }
  if (c >= 0x10000 && c <= 0x10ffff) {
    write_buff_char_4 ((char)(0xf0 + (c >> 18)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }
}


static int win_to_utf8 (const char *s, int len, bool escape) {
  int i;
  int state = 0;
  int save_pos = -1;
  int cur_num = 0;
  for (i = 0; i < len; i++) {
    if (state == 0 && s[i] == '&') {
      save_pos = cur_buff_len;
      cur_num = 0;
      state++;
    } else if (state == 1 && s[i] == '#') {
      state++;
    } else if (state == 2 && s[i] >= '0' && s[i] <= '9') {
      if (cur_num < 0x20000) {
        cur_num = s[i] - '0' + cur_num * 10;
      }
    } else if (state == 2 && s[i] == ';') {
      state++;
    } else {
      state = 0;
    }
    if (state == 3 && 0xd800 <= cur_num && cur_num <= 0xdfff){
      cur_num = 32;
    }
    if (state == 3 && (!escape || (cur_num >= 32 && cur_num != 33 && cur_num != 34 && cur_num != 36 && cur_num != 39 && cur_num != 60 && cur_num != 62 && cur_num != 92 && cur_num != 8232 && cur_num != 8233 && cur_num < 0x20000))) {
      write_buff_set_pos (save_pos);
      php_assert (save_pos == cur_buff_len);
      (escape ? write_char_utf8 : write_char_utf8_no_escape) (cur_num);
    } else if (state == 3 && cur_num >= 0x10000) {
      write_char_utf8 (win_to_utf8_convert[(unsigned char)s[i]]);
      write_buff_char_pos ('$', save_pos);
    } else {
      write_char_utf8 (win_to_utf8_convert[(unsigned char)s[i]]);
    }
    if (state == 3) {
      state = 0;
    }
  }
  return cur_buff_len;
}


char ws[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

static inline bool is_html_opt_symb (char c) {
  return (c == '<' || c == '>' || c == '(' || c == ')' || c == '{' || c == '}' || c == '/' || c== '"' || c== ':' || c== ',' || c== ';');
}
static inline bool is_space (char c) {
  return ws[(unsigned char)c];
}

static inline bool is_linebreak (char c) {
  return c == '\n';
}

static inline bool is_pre_tag (const char *s) {
  if (s[0] == '<') {
    if (s[1] == 'p') {
      return s[2] == 'r' && s[3] == 'e' && s[4] == '>';
    } else if (s[1] == 'c') {
      return s[2] == 'o' && s[3] == 'd' && s[4] == 'e' && s[5] == '>';
    } else if (s[1] == '/') {
      if (s[2] == 'p') {
        return -(s[3] == 'r' && s[4] == 'e' && s[5] == '>');
      } else if (s[2] == 'c') {
        return -(s[3] == 'o' && s[4] == 'd' && s[5] == 'e' && s[6] == '>');
      }
    }
  }
  return false;
}

string f$vk_utf8_to_win (const string &text, int max_len, bool exit_on_error) {
  init_buff();
  int r = utf8_to_win (text.c_str(), text.size(), max_len, exit_on_error);
  if (r >= 0) {
    return finish_buff (max_len);
  } else {
    if (!max_len || text.size() <= (dl::size_type)max_len) {
      return text;
    }
    return string (text.c_str(), max_len);
  }
}

string f$vk_win_to_utf8(const string &text, bool escape) {
  init_buff();
  win_to_utf8 (text.c_str(), text.size(), escape);
  return finish_buff (0);
}

string f$vk_flex (const string &name, const string &case_name, int sex, const string &type, int lang_id) {
  if (name.size() > (1 << 10)) {
    php_warning ("Name has length %d and is too long in function vk_flex", name.size());
    return name;
  }
  if (case_name[0] == 0 || !strcmp (case_name.c_str(), "Nom")) {
    return name;
  }

  if ((unsigned int)lang_id >= (unsigned int)LANG_NUM) {
    php_warning ("Unknown lang id %d in function vk_flex", lang_id);
    return name;
  }

  if (!langs[lang_id]) {
    return name;
  }

  lang *cur_lang = langs[lang_id];

  int t = -1;
  if (!strcmp (type.c_str(), "names")) {
    if (cur_lang->names_start < 0) {
      return name;
    }
    t = cur_lang->names_start;
  } else if (!strcmp (type.c_str(), "surnames")) {
    if (cur_lang->surnames_start < 0) {
      return name;
    }
    t = cur_lang->surnames_start;
  } else {
    php_warning ("Unknown type %s in function vk_flex", type.c_str());
    return name;
  }
  php_assert (t >= 0);

  if (sex != 1) {
    sex = 0;
  }
  int ca = -1;
  for (int i = 0; i < CASES_NUM && i < cur_lang->cases_num; i++) {
    if (!strcmp (cases_names[i], case_name.c_str())) {
      ca = i;
      break;
    }
  }
  if (ca == -1) {
    php_warning ("Unknown case \"%s\" in function vk_flex", case_name.c_str());
    return name;
  }

  int p = 0;
  int wp = 0;
  int name_len = name.size();
  while (p < name_len) {
    int pp = p;
    while (pp < name_len && name[pp] != '-') {
      pp++;
    }
    int hyphen = (name[pp] == '-');
    int tt = t;
    int best = -1;
    int save_pp = pp;
    int new_tt;
    int isf = 0;
    if (pp - p > 0) {
      const char *fle = cur_lang->flexible_symbols;
      while (*fle) {
        if (*fle == name[pp - 1]) {
          isf = 1;
          break;
        }
        fle++;
      }
    }
    while (isf) {
      php_assert (tt >= 0);
      if (cur_lang->nodes[tt].tail_len >= 0 && (!cur_lang->nodes[tt].hyphen || hyphen)) {
        best = tt;
      }
      unsigned char c;
      if (pp == p - 1) {
        break;
      }
      pp--;
      if (pp < p) {
        c = 0;
      } else {
        c = name[pp];
      }
      new_tt = -1;
      int l = cur_lang->nodes[tt].children_start;
      int r = cur_lang->nodes[tt].children_end;
      if (r - l <= 4) {
        for (int i = l; i < r; i++) {
          if (cur_lang->children[2 * i] == c) {
            new_tt = cur_lang->children[2 * i + 1];
            break;
          }
        }
      } else {
        int x;
        while (r - l > 1) {
          x = (r + l) >> 1;
          if (cur_lang->children[2 * x] <= c) {
            l = x;
          } else {
            r = x;
          }
        }
        if (cur_lang->children[2 * l] == c) {
          new_tt = cur_lang->children[2 * l + 1];
        }
      }
      if (new_tt == -1) {
        break;
      } else {
        tt = new_tt;
      }
    }
    if (best == -1) {
      memcpy (buff + wp, name.c_str() + p, save_pp - p);
      wp += (save_pp - p);
    } else {
      int r = -1;
      if (!sex) {
        r = cur_lang->nodes[best].male_endings;
      } else {
        r = cur_lang->nodes[best].female_endings;
      }
      if (r < 0 || !cur_lang->endings[r * cur_lang->cases_num + ca]) {
        memcpy (buff + wp, name.c_str() + p, save_pp - p);
        wp += (save_pp - p);
      } else {
        int ml = save_pp - p - cur_lang->nodes[best].tail_len;
        if (ml < 0) {
          ml = 0;
        }
        memcpy (buff + wp, name.c_str() + p, ml);
        wp += ml;
        strcpy (buff + wp, cur_lang->endings[r * cur_lang->cases_num + ca]);
        wp += (int)strlen (cur_lang->endings[r * cur_lang->cases_num + ca]);
      }
    }
    if (hyphen) {
      buff[wp++] = '-';
    } else {
      buff[wp++] = 0;
    }
    p = save_pp + 1;
  }
  buff[wp] = 0;

  return string (buff, (dl::size_type)strlen (buff));
}

OrFalse<string> f$vk_json_encode (const var &v) {
  return f$json_encode (v, 0, true);
}

string f$vk_whitespace_pack (const string &str, bool html_opt) {
  const char *text = str.c_str();
  int level = 0;
  const char *ctext = text;
  const char *start = text;

  init_buff();
  while (*text) {
    if (is_space (*text) && !level) {
      int linebreak = 0;
      while (is_space (*text)) {
        if (is_linebreak (*text)) {
          linebreak = 1;
        }
        text++;
      }
      if (!html_opt || (ctext != start && !is_html_opt_symb (ctext[-1]) && *text && !is_html_opt_symb (*text))) {
        write_buff_char (linebreak ? '\n' : ' ');
      }
    } else {
      while (1) {
        while ((level || !is_space (*text)) && *text) {
          level += is_pre_tag (text);
          if (level < 0) {
            level = 1000000000;
          }
          text++;
        }
        if (!html_opt && *text && !is_space (text[1])) {
          text++;
        } else {
          break;
        }
      }
      write_buff (ctext, (int)(text - ctext));
    }
    ctext = text;
  }
  return finish_buff (0);
}


string f$vk_sp_simplify (const string& s) {
  sp_init();
  char* t = sp_simplify (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_full_simplify (const string& s) {
  sp_init();
  char* t = sp_full_simplify (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_deunicode (const string& s) {
  sp_init();
  char* t = sp_deunicode (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_to_upper (const string& s) {
  sp_init();
  char* t = sp_to_upper (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_to_lower (const string& s) {
  sp_init();
  char* t = sp_to_lower (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_to_sort (const string& s) {
  sp_init();
  char* t = sp_sort (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_remove_repeats (const string& s) {
  sp_init();
  char* t = sp_remove_repeats (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_to_cyrillic (const string& s) {
  sp_init();
  char* t = sp_to_cyrillic (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}

string f$vk_sp_words_only (const string& s) {
  sp_init();
  char* t = sp_words_only (s.c_str());
  if (!t) {
    return string ();
  }

  return string (t, (string::size_type)strlen(t));
}
