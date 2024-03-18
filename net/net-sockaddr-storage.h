// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_SOCKADDR_STORAGE_H
#define KDB_NET_NET_SOCKADDR_STORAGE_H

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define cast_sockaddr_storage_impl(storage, family, type)                                                                                                      \
  ({                                                                                                                                                           \
    assert((storage)->ss_family == family);                                                                                                                    \
    (type)(storage);                                                                                                                                           \
  })

#define cast_sockaddr_storage_to_inet(storage) cast_sockaddr_storage_impl(storage, AF_INET, struct sockaddr_in *)
#define const_cast_sockaddr_storage_to_inet(storage) cast_sockaddr_storage_impl(storage, AF_INET, const struct sockaddr_in *)
#define cast_sockaddr_storage_to_inet6(storage) cast_sockaddr_storage_impl(storage, AF_INET6, struct sockaddr_in6 *)
#define const_cast_sockaddr_storage_to_inet6(storage) cast_sockaddr_storage_impl(storage, AF_INET6, const struct sockaddr_in6 *)
#define cast_sockaddr_storage_to_unix(storage) cast_sockaddr_storage_impl(storage, AF_UNIX, struct sockaddr_un *)
#define const_cast_sockaddr_storage_to_unix(storage) cast_sockaddr_storage_impl(storage, AF_UNIX, const struct sockaddr_un *)

static inline uint16_t inet_sockaddr_port(const struct sockaddr_storage *storage) {
  if (storage->ss_family == AF_INET) {
    const struct sockaddr_in *inet_addr = const_cast_sockaddr_storage_to_inet(storage);
    return ntohs(inet_addr->sin_port);
  }

  if (storage->ss_family == AF_UNIX) {
    const struct sockaddr_un *unix_addr = const_cast_sockaddr_storage_to_unix(storage);
    const size_t path_length = strlen(unix_addr->sun_path);
    assert(path_length + 1 + sizeof(uint16_t) <= sizeof(unix_addr->sun_path));
    return *(uint16_t *)(&unix_addr->sun_path[path_length + 1]);
  }

  assert(0);
}

static inline uint32_t inet_sockaddr_address(const struct sockaddr_storage *storage) {
  if (storage->ss_family == AF_INET) {
    const struct sockaddr_in *inet_addr = const_cast_sockaddr_storage_to_inet(storage);
    return ntohl(inet_addr->sin_addr.s_addr);
  }

  if (storage->ss_family == AF_UNIX) {
    return 0x7f000001;
  }

  assert(0);

  return 0;
}

static inline const uint8_t *inet6_sockaddr_address(const struct sockaddr_storage *storage) {
  const struct sockaddr_in6 *inet6_addr = const_cast_sockaddr_storage_to_inet6(storage);
  return inet6_addr->sin6_addr.s6_addr;
}

static inline uint16_t inet6_sockaddr_port(const struct sockaddr_storage *storage) {
  const struct sockaddr_in6 *inet6_addr = const_cast_sockaddr_storage_to_inet6(storage);
  return ntohs(inet6_addr->sin6_port);
}

static inline const char *unix_sockaddr_address(const struct sockaddr_storage *storage) {
  const struct sockaddr_un *unix_addr = const_cast_sockaddr_storage_to_unix(storage);
  return unix_addr->sun_path;
}

static inline struct sockaddr_storage make_inet_sockaddr_storage(uint32_t address, uint16_t port) {
  struct sockaddr_storage storage;
  storage.ss_family = AF_INET;
  struct sockaddr_in *sockaddr = cast_sockaddr_storage_to_inet(&storage);

  sockaddr->sin_addr.s_addr = htonl(address);
  sockaddr->sin_port = htons(port);

  return storage;
}

static inline struct sockaddr_storage make_inet6_sockaddr_storage(uint8_t address[16], uint16_t port) {
  struct sockaddr_storage storage;
  storage.ss_family = AF_INET6;
  struct sockaddr_in6 *sockaddr = cast_sockaddr_storage_to_inet6(&storage);

  memcpy(sockaddr->sin6_addr.s6_addr, address, sizeof(sockaddr->sin6_addr.s6_addr));
  sockaddr->sin6_port = htons(port);

  return storage;
}

static inline struct sockaddr_storage make_unix_sockaddr_storage(const char *address, uint16_t port) {
  struct sockaddr_storage storage;
  storage.ss_family = AF_UNIX;
  struct sockaddr_un *sockaddr = cast_sockaddr_storage_to_unix(&storage);

  const size_t written = snprintf(sockaddr->sun_path, sizeof(sockaddr->sun_path), "%s", address);
  assert(written + 1 + sizeof(port) <= sizeof(sockaddr->sun_path));
  *((uint16_t *)&sockaddr->sun_path[written + 1]) = port;

  return storage;
}

#define UNIX_PATH_MAX 108
#define SOCKADDR_STORAGE_BUFFER_SIZE (UNIX_PATH_MAX)
const char *sockaddr_storage_to_string(const struct sockaddr_storage *storage);
const char *sockaddr_storage_to_buffer(const struct sockaddr_storage *storage, char buffer[UNIX_PATH_MAX]);

#endif // KDB_NET_NET_SOCKADDR_STORAGE_H
