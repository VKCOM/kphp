// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_TCP_CONNECTIONS_H__
#define __NET_TCP_CONNECTIONS_H__

#include <sys/cdefs.h>

#define MAX_TCP_RECV_BUFFERS 8
#define MAX_TCP_RECV_BUFFER_SIZE 1024

#include "net/net-connections.h"

int tcp_server_writer(struct connection *c);
int tcp_free_connection_buffers(struct connection *c);
int tcp_server_reader(struct connection *c);
int tcp_server_reader_till_end(struct connection *c);
int tcp_aes_crypto_decrypt_input(struct connection *c);
int tcp_aes_crypto_encrypt_output(struct connection *c);
int tcp_aes_crypto_needed_output_bytes(struct connection *c);

extern int tcp_buffers;

#endif
