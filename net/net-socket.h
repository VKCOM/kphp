// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_SOCKET_H
#define KDB_NET_NET_SOCKET_H

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdbool.h>

#define PRIVILEGED_TCP_PORTS 1024

extern in_addr settings_addr;
extern int backlog;

#define SM_DGRAM 0x00000001
#define SM_IPV6 0x00000002
#define SM_IPV6_ONLY 0x00000004
#define SM_LOWPRIO 0x00000008
#define SM_REUSE 0x00000010
#define SM_UNIX 0x00000020
#define SM_REUSEPORT 0x00000040
#define SM_SPECIAL 0x00010000
#define SM_NOQACK 0x00020000
#define SM_RAWMSG 0x00040000

void set_backlog(const char *arg);
int server_socket(int port, struct in_addr in_addr, int backlog, int mode);
int server_socket_unix(const struct sockaddr_un *addr, int backlog, int mode);
int client_socket(const struct sockaddr_in *addr, int mode);
int client_socket_ipv6(const struct sockaddr_in6 *addr, int mode);
int client_socket_unix(const struct sockaddr_un *addr, int mode);

const char *unix_socket_path(const char *directory, const char *owner, uint16_t port);
int prepare_unix_socket_directory(const char *directory, const char *username, const char *groupname);
int prepare_unix_socket(const char *path, const char *username, const char *groupname);

bool set_fd_nonblocking(int fd);

static inline int is_4in6(const unsigned char ipv6[16]) {
  return !*((long long *)ipv6) && ((int *)ipv6)[2] == -0x10000;
}

static inline unsigned extract_4in6(const unsigned char ipv6[16]) {
  return (((unsigned *)ipv6)[3]);
}

static inline void set_4in6(unsigned char ipv6[16], unsigned ip) {
  *(long long *)ipv6 = 0;
  ((int *)ipv6)[2] = -0x10000;
  ((unsigned *)ipv6)[3] = ip;
}

static inline int socket_ioctl(int fd, unsigned long request) {
  int value;

  if (ioctl(fd, request, &value) == -1) {
    return -1;
  }

  return value;
}

#endif // KDB_NET_NET_SOCKET_H
