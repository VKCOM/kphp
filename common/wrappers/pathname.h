// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_PATHNAME_H
#define KDB_COMMON_PATHNAME_H

#include <string.h>

static inline const char *kbasename(const char *path) {
  const char *p = strrchr(path, '/');
  return p ? p + 1 : path;
}

#endif // KDB_COMMON_PATHNAME_H
