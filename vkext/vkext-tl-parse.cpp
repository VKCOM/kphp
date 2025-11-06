// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string.h>

#include "vkext/vkext-rpc.h"

struct tl {
  char *error;
  int errnum;
};
struct tl tl;

void tl_parse_init() {
  if (tl.error) {
    free(tl.error);
    tl.error = 0;
  }
  tl.errnum = 0;
}

int tl_parse_int() {
  if (tl.error) {
    return -1;
  }
  return do_rpc_fetch_int(&tl.error);
}

int tl_lookup_int() {
  if (tl.error) {
    return -1;
  }
  return do_rpc_lookup_int(&tl.error);
}

long long tl_parse_long() {
  if (tl.error) {
    return -1;
  }
  return do_rpc_fetch_long(&tl.error);
}

double tl_parse_double() {
  if (tl.error) {
    return -1;
  }
  return do_rpc_fetch_double(&tl.error);
}

float tl_parse_float() {
  if (tl.error) {
    return -1;
  }
  return do_rpc_fetch_float(&tl.error);
}

int tl_parse_string(char **s) {
  if (tl.error) {
    return -1;
  }
  int len = do_rpc_fetch_string(s);
  if (len < 0) {
    tl.error = strdup(*s);
    *s = 0;
    return -1;
  }
  char *t = static_cast<char *>(malloc(len + 1));
  memcpy(t, *s, len);
  t[len] = 0;
  *s = t;
  return len;
}

int tl_eparse_string(char **s) {
  if (tl.error) {
    return -1;
  }
  int len = do_rpc_fetch_string(s);
  if (len < 0) {
    tl.error = strdup(*s);
    *s = 0;
    return -1;
  }
  char *t = static_cast<char *>(emalloc (len + 1));
  memcpy(t, *s, len);
  t[len] = 0;
  *s = t;
  return len;
}

std::string tl_parse_string() {
  std::string res;
  char *s = nullptr;
  int l = tl_parse_string(&s);
  if (l < 0) {
    return res;
  }
  res.assign(s);
  free(s);
  return res;
}

char *tl_parse_error() {
  return tl.error;
}

void tl_set_error(const char *error) {
  if (tl.error) {
    return;
  }
  tl.error = strdup(error);
}

void tl_parse_end() {
  if (tl.error) {
    return;
  }
  if (!do_rpc_fetch_eof(const_cast<const char**>(&tl.error))) {
    tl_set_error("Extra data");
  }
}

int tl_parse_save_pos() {
  char *error = 0;
  int r = do_rpc_fetch_get_pos(&error);
  if (error) {
    tl_set_error(error);
    return 0;
  } else {
    return r;
  }
}

int tl_parse_restore_pos(int pos) {
  char *error = 0;
  do_rpc_fetch_set_pos(pos, &error);
  if (error) {
    tl_set_error(error);
    return 0;
  } else {
    return 1;
  }
}

void tl_parse_clear_error() {
  if (tl.error) {
    free(tl.error);
    tl.error = 0;
  }
}
