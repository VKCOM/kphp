// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_SOCKET_OPTIONS_H
#define KDB_NET_NET_SOCKET_OPTIONS_H

#include <stdbool.h>
#include <sys/cdefs.h>

bool socket_error(int socket, int *error);

bool socket_disable_ip_v6_only(int socket);
bool socket_disable_linger(int socket);
bool socket_disable_tcp_quickack(int socket);
bool socket_disable_unix_passcred(int socket);
bool socket_enable_ip_receive_packet_info(int socket);
bool socket_enable_ip_v6_receive_packet_info(int socket);
bool socket_enable_ip_receive_errors(int socket);
bool socket_enable_ip_v6_only(int socket);
bool socket_enable_keepalive(int socket);
bool socket_enable_reuseaddr(int socket);
bool socket_enable_reuseport(int socket);
bool socket_enable_tcp_nodelay(int socket);
bool socket_enable_unix_passcred(int socket);
bool socket_set_tcp_window_clamp(int socket, int size);
void socket_maximize_rcvbuf(int socket, int max);
void socket_maximize_sndbuf(int socket, int max);
bool socket_get_domain(int socket, int *domain);

#endif // KDB_NET_NET_SOCKET_OPTIONS_H
