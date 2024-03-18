// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-socket-options.h"

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common/kprintf.h"

#define MAX_UDP_SENDBUF_SIZE (16 * 1024 * 1024)
#define MAX_UDP_RECVBUF_SIZE (16 * 1024 * 1024)

bool socket_error(int socket, int *error) {
  socklen_t error_length = sizeof(*error);
  return !getsockopt(socket, SOL_SOCKET, SO_ERROR, error, &error_length);
}

bool socket_disable_ip_v6_only(int socket) {
  int disable = 0;
  return !setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, &disable, sizeof(disable));
}

bool socket_disable_linger(int socket) {
  struct linger disable = {0, 0};
  return !setsockopt(socket, SOL_SOCKET, SO_LINGER, &disable, sizeof(disable));
}

bool socket_disable_tcp_quickack(int socket) {
#if defined(TCP_QUICKACK)
  int disable = 0;
  return !setsockopt(socket, IPPROTO_TCP, TCP_QUICKACK, &disable, sizeof(disable));
#else
  return false && socket;
#endif
}

bool socket_disable_unix_passcred(int socket) {
#if defined(SO_PASSCRED)
  int disable = 0;
  return !setsockopt(socket, SOL_SOCKET, SO_PASSCRED, &disable, sizeof(disable));
#else
  return false && socket;
#endif
}

bool socket_enable_keepalive(int socket) {
  int enable = 1;
  return !setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
}

bool socket_enable_ip_receive_packet_info(int socket) {
  int enable = 1;
  return !setsockopt(socket, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
}

bool socket_enable_ip_receive_errors(int socket) {
#if defined(SOL_IP) && defined(IP_RECVERR)
  int enable = 1;
  return !setsockopt(socket, SOL_IP, IP_RECVERR, &enable, sizeof(enable));
#else
  return false && socket;
#endif
}

bool socket_enable_ip_v6_receive_packet_info(int socket) {
#if defined(IPV6_RECVPKTINFO)
  int enable = 1;
  return !setsockopt(socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &enable, sizeof(enable));
#else
  return false && socket;
#endif
}

bool socket_enable_ip_v6_only(int socket) {
  int enable = 1;
  return !setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, &enable, sizeof(enable));
}

bool socket_enable_reuseaddr(int socket) {
  int enable = 1;
  return !setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

bool socket_enable_reuseport(int socket) {
#if defined(SO_REUSEPORT)
  int enable = 1;
  return !setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
#else
  return false && socket;
#endif
}

bool socket_enable_tcp_nodelay(int socket) {
  int enable = 1;
  return !setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

bool socket_enable_unix_passcred(int socket) {
#if defined(SO_PASSCRED)
  int enable = 1;
  return !setsockopt(socket, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));
#else
  return false && socket;
#endif
}

bool socket_set_tcp_window_clamp(int socket, int size) {
#if defined(TCP_WINDOW_CLAMP)
  return !setsockopt(socket, IPPROTO_TCP, TCP_WINDOW_CLAMP, &size, sizeof(size));
#else
  return false && socket && size;
#endif
}

void socket_maximize_sndbuf(int socket, int max) {
  socklen_t intsize = sizeof(int);
  int last_good = 0;
  int min, avg;
  int old_size;

  if (max <= 0) {
    max = MAX_UDP_SENDBUF_SIZE;
  }

  /* Start with the default size. */
  if (getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize)) {
    if (verbosity > 0) {
      perror("getsockopt (SO_SNDBUF)");
    }
    return;
  }

  /* Binary-search for the real maximum. */
  min = last_good = old_size;
  max = MAX_UDP_SENDBUF_SIZE;

  while (min <= max) {
    avg = ((unsigned int)min + max) / 2;
    if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &avg, intsize) == 0) {
      last_good = avg;
      min = avg + 1;
    } else {
      max = avg - 1;
    }
  }

  vkprintf(4, "<%d send buffer was %d, now %d\n", socket, old_size, last_good);
}

void socket_maximize_rcvbuf(int socket, int max) {
  socklen_t intsize = sizeof(int);
  int last_good = 0;
  int min, avg;
  int old_size;

  if (max <= 0) {
    max = MAX_UDP_RECVBUF_SIZE;
  }

  /* Start with the default size. */
  if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &old_size, &intsize)) {
    if (verbosity > 0) {
      perror("getsockopt (SO_RCVBUF)");
    }
    return;
  }

  /* Binary-search for the real maximum. */
  min = last_good = old_size;
  max = MAX_UDP_RECVBUF_SIZE;

  while (min <= max) {
    avg = ((unsigned int)min + max) / 2;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &avg, intsize) == 0) {
      last_good = avg;
      min = avg + 1;
    } else {
      max = avg - 1;
    }
  }

  vkprintf(4, ">%d receive buffer was %d, now %d\n", socket, old_size, last_good);
}
