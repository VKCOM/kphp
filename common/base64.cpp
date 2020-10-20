#include "common/base64.h"

#include <assert.h>
#include <string.h>

#include "common/algorithms/arithmetic.h"

static inline unsigned char next_input_uchar (const unsigned char *const input, int ilen, int *i) {
  if (*i >= ilen) { return 0; }
  return input[(*i)++];
}

static const char* const symbols64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

int base64_encode (const unsigned char *const input, int ilen, char *output, int olen) {
  int i, j = 0;
  char buf[4];
  for (i = 0; i < ilen; ) {
    int old_i = i;
    int o = next_input_uchar (input, ilen, &i);
    o <<= 8;
    o |= next_input_uchar (input, ilen, &i);
    o <<= 8;
    o |= next_input_uchar (input, ilen, &i);
    int l = i - old_i;
    assert (l > 0 && l <= 3);
    int u;
    for (u = 3; u >= 0; u--) {
      buf[u] = symbols64[o & 63];
      o >>= 6;
    }
    if (l == 1) { 
      buf[2] = buf[3] = '='; 
    }
    else if (l == 2) { 
      buf[3] = '='; 
    }
    if (j + 3 >= olen) {
      return -1;
    }
    memcpy (&output[j], buf, 4);
    j += 4;
  }
  if (j >= olen) {
    return -1;
  }
  output[j++] = 0;
  return 0;
}

int base64_decode (const char *const input, unsigned char *output, int olen) {
  static int tbl_symbols64_initialized = 0;
  static char tbl_symbols64[256];
  int ilen = strlen (input);
  int i, j = 0;
  if (!tbl_symbols64_initialized) {
    memset (tbl_symbols64, 0xff, 256);
    for (i = 0; i <= 64; i++) {
      tbl_symbols64[(int) symbols64[i]] = i; 
    }
    tbl_symbols64_initialized = 1;
  }
  if (ilen & 3) {
    return -2;
  }
  char buf[3];
  for (i = 0; i < ilen; i += 4) {
    int o = 0, l = 3, u = 0;
    do {
      int c = tbl_symbols64[(unsigned char) input[i+u]];
      if (c < 0) {
        return -3;
      }
      if (c == 64) {
        switch (u) {
          case 0:
          case 1:
            return -4;
          case 2:
            if (tbl_symbols64[(unsigned char) input[i+3]] != 64) {
              return -5;
            }
            o <<= 12;
            l = 1;
            break;
          case 3:
            o <<= 6;
            l = 2;
            break;
        }
        break;
      }
      o <<= 6;
      o |= c;
    } while (++u < 4);
    u = 2;
    do {
      buf[u] = o & 255;
      o >>= 8;
    } while (--u >= 0);
    if (j + l > olen) {
      return -1;
    }
    memcpy (&output[j], buf, l);
    j += l;
  }
  return j;
}

int base64_to_base64url (const char *const input, char *output, int olen) {
  int i = 0;
  while (input[i] && i < olen) {
    if (input[i] == '+') {
      output[i] = '-';
    } else if (input[i] == '/') {
      output[i] = '_';
    } else if (input[i] != '=') {
      output[i] = input[i];
    } else {
      break;
    }
    i++;
  }
  if (i >= olen) {
    return -1;
  }
  output[i] = 0;
  return 0;
}

int base64url_to_base64 (const char *const input, char *output, int olen) {
  int i = 0;
  while (input[i] && i < olen) {
    if (input[i] == '-') {
      output[i] = '+';
    } else if (input[i] == '_') {
      output[i] = '/';
    } else {
      output[i] = input[i];
    }
    i++;
  }
  if (align4(i) >= olen) {
    return -1;
  }
  while (i & 3) {
    output[i++] = '=';
  }
  output[i] = 0;
  assert (i < olen);
  return 0;
}


static const char* const url_symbols64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int base64url_encode (const unsigned char *const input, int ilen, char *output, int olen) {
  int i, j = 0;
  char buf[4];
  for (i = 0; i < ilen; ) {
    int old_i = i;
    int o = next_input_uchar (input, ilen, &i);
    o <<= 8;
    o |= next_input_uchar (input, ilen, &i);
    o <<= 8;
    o |= next_input_uchar (input, ilen, &i);
    int l = i - old_i;
    assert (l > 0 && l <= 3);
    int u;
    for (u = 3; u >= 0; u--) {
      buf[u] = url_symbols64[o & 63];
      o >>= 6;
    }
    l++;
    if (j + l >= olen) {
      return -1;
    }
    memcpy (&output[j], buf, l);
    j += l;
  }
  if (j >= olen) {
    return -1;
  }
  output[j++] = 0;
  return 0;
}

int base64url_decode (const char *const input, unsigned char *output, int olen) {
  static int tbl_url_symbols64_initialized = 0;
  static char tbl_url_symbols64[256];
  int ilen = strlen (input);
  int i, j = 0;
  if (!tbl_url_symbols64_initialized) {
    memset (tbl_url_symbols64, 0xff, 256);
    for (i = 0; i < 64; i++) {
      tbl_url_symbols64[(int) url_symbols64[i]] = i; 
    }
    tbl_url_symbols64_initialized = 1;
  }
  char buf[3];
  for (i = 0; i < ilen; i += 4) {
    int o = 0, l = 3, u = 0;
    do {
      if (i + u >= ilen) {
        switch (u) {
          case 0:
          case 1:
            return -4;
          case 2:
            o <<= 12;
            l = 1;
            break;
          case 3:
            o <<= 6;
            l = 2;
            break;
        }
        break;
      }
      int c = tbl_url_symbols64[(unsigned char) input[i+u]];
      if (c < 0) {
        return -3;
      }
      o <<= 6;
      o |= c;
    } while (++u < 4);
    u = 2;
    do {
      buf[u] = o & 255;
      o >>= 8;
    } while (--u >= 0);
    if (j + l > olen) {
      return -1;
    }
    memcpy (&output[j], buf, l);
    j += l;
  }
  return j;
}

