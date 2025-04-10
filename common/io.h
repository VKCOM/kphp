// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_IO_H
#define KDB_COMMON_IO_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/uio.h>
#include <unistd.h>

static inline bool read_exact(int fd, void* buffer, size_t count) {
  size_t total = 0;
  do {
    const ssize_t part = read(fd, (char*)(buffer) + total, count - total);
    if (part > 0) {
      total += part;
    } else if (part == 0 && count > 0) {
      return false;
    } else if (part == -1) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }
  } while (total < count);

  return true;
}

static inline bool write_exact(int fd, const void* buffer, size_t size) {
  ssize_t total = 0;
  do {
    const ssize_t part = write(fd, (const char*)(buffer) + total, size - total);
    if (part > 0) {
      total += part;
    } else if (part == 0 && size > 0) {
      return false;
    } else if (part == -1) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }
  } while (total > 0 && (size_t)total < size);

  return true;
}

#endif // KDB_COMMON_IO_H
