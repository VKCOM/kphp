// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-sockaddr-storage.h"

#include <sys/un.h>

#include <assert.h>
#include <stdio.h>

const char *sockaddr_storage_to_string(const struct sockaddr_storage *storage) {
  static char buffer[SOCKADDR_STORAGE_BUFFER_SIZE];

  return sockaddr_storage_to_buffer(storage, buffer);
}

const char *sockaddr_storage_to_buffer(const struct sockaddr_storage *storage, char buffer[SOCKADDR_STORAGE_BUFFER_SIZE]) {
  switch (storage->ss_family) {
    case AF_INET: {
      struct sockaddr_in *inet_addr = (struct sockaddr_in *)storage;
      snprintf(buffer, SOCKADDR_STORAGE_BUFFER_SIZE, "%s:%d", inet_ntoa(inet_addr->sin_addr), ntohs(inet_addr->sin_port));
      break;
    }
    case AF_INET6: {
      struct sockaddr_in6 *inet6_addr = (struct sockaddr_in6 *)storage;
      snprintf(buffer, SOCKADDR_STORAGE_BUFFER_SIZE, "%s:%d", inet_ntop(AF_INET6, inet6_addr, buffer, SOCKADDR_STORAGE_BUFFER_SIZE),
               ntohs(inet6_addr->sin6_port));
      break;
    }
    case AF_UNIX: {
      struct sockaddr_un *unix_addr = (struct sockaddr_un *)storage;
      snprintf(buffer, SOCKADDR_STORAGE_BUFFER_SIZE, "%s", unix_addr->sun_path);
      break;
    }
    default:
      assert(0);
  }

  return buffer;
}
