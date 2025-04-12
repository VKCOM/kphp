// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <assert.h>
#include <climits>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static inline bool mkdir_recursive(const char *s, mode_t mode) {
  assert(s != NULL);

  size_t len = strlen(s);
  assert(len < PATH_MAX);

  char path[PATH_MAX];
  memcpy(path, s, len + 1);

  if (path[len - 1] != '/') {
    assert(len + 1 < PATH_MAX);
    path[len] = '/';
    path[len + 1] = '\0';
  }

  char *it = path;
  while ((it = strchr(it + 1, '/'))) {
    *it = 0;

    if (mkdir(path, mode) != 0 && errno != EEXIST) {
      return false;
    }

    *it = '/';
  }

  return true;
}
