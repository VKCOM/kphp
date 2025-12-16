// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-tcp-rpc-client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/precise-time.h"
#include "common/tl/constants/common.h"

#include "net/net-buffers.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-events.h"
#include "net/net-ifnet.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-tcp-connections.h"
#include "net/net-tcp-rpc-common.h"

/*
 *
 *                BASIC RPC CLIENT INTERFACE
 *
 */

conn_type_t get_default_tcp_rpc_client_conn_type() {
  conn_type_t res = {
    .magic = CONN_FUNC_MAGIC,
    .flags = C_RAWMSG,
    .title = "rpc_client",
    .accept = server_failed,
    .init_accepted = server_failed,
    .create_outbound = NULL,
    .run = server_read_write,
    .reader = tcp_server_reader,
    .writer = tcp_server_writer,
    .close = tcp_rpcc_close_connection,
    .free_buffers = tcp_free_connection_buffers,
    .parse_execute = tcp_rpcc_parse_execute,
    .init_outbound = tcp_rpcc_init_outbound,
    .connected = tcp_rpcc_connected,
    .wakeup = server_noop,
    .alarm = NULL,
    .ready_to_write = NULL,
    .check_ready = tcp_rpc_client_check_ready,
    .wakeup_aio = NULL,
    .data_received = NULL,
    .data_sent = NULL,
    .ancillary_data_received = NULL,
    .flush = tcp_rpcc_flush,
    .crypto_init = aes_crypto_init,
    .crypto_free = aes_crypto_free,
    .crypto_encrypt_output = tcp_aes_crypto_encrypt_output,
    .crypto_decrypt_input = tcp_aes_crypto_decrypt_input,
    .crypto_needed_output_bytes = tcp_aes_crypto_needed_output_bytes,
  };
  return res;
}

conn_type_t ct_tcp_rpc_client = get_default_tcp_rpc_client_conn_type();


int tcp_rpcc_default_execute (struct connection *c, int op, raw_message_t *raw) {
  tvkprintf(net_connections, 3, "rpcc_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);
  if (op == TL_RPC_PING && raw->total_bytes == 12) {
    c->last_response_time = precise_now;    
    int Q[12];
    assert (rwm_fetch_data (raw, Q, 12) == 12);
    int P[12];
    P[0] = TL_RPC_PONG;
    P[1] = Q[1];
    P[2] = Q[2];
    
    tvkprintf(net_connections, 4, "received ping from %s (val = %lld)\n", sockaddr_storage_to_string(&c->remote_endpoint), *(long long *)(P + 1));
    tcp_rpc_conn_send_data (c, 12, P);
    return 0;
  }
  c->last_response_time = precise_now;    
  return 0;
}

static int tcp_rpcc_process_nonce_packet (struct connection *c, raw_message_t *msg) {
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  struct tcp_rpc_nonce_packet P{};
  int res;

  if (D->packet_num != -2 || D->packet_type != RPC_NONCE) {
    return -2;
  }
  if (D->packet_len < sizeof (P) || D->packet_len >= 1024) {
    return -3;
  }
  int excess_data_size = D->packet_len - sizeof(P); // fields from newer protocol version

  assert (rwm_fetch_data (msg, &P, sizeof(P)) == sizeof(P));
  assert (rwm_fetch_data (msg, 0, excess_data_size) == excess_data_size);
  tvkprintf(net_connections, 4, "Processing nonce packet, crypto schema: %d, version: %d, excess_data: %d, key select: %d\n", P.crypto_schema, P.protocol_version, excess_data_size, P.key_select);

  if (P.protocol_version > 1) { // server must not reply with version > client
    return -3;
  }

  switch (P.crypto_schema) {
  case RPC_CRYPTO_NONE:
    if (P.key_select) {
      return -3;
    }
    if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
      if (D->crypto_flags & RPC_CRYPTO_ALLOW_ENCRYPTED) {
        assert (!c->out_p.total_bytes);
      }
      D->crypto_flags = 1;
    } else {
      return -5;
    }
    break;
  case RPC_CRYPTO_AES:
    if (!P.key_select || P.key_select != get_crypto_key_id (default_aes_key)) {
      return -3;
    }
    if (!(D->crypto_flags & RPC_CRYPTO_ALLOW_ENCRYPTED)) {
      return -5;
    }
    if (abs (P.crypto_ts - D->nonce_time) > 30) {
      return -6;        //less'om
    }
    res = TCP_RPCC_FUNC(c)->rpc_start_crypto (c, &P);
    if (res < 0) {
      return -6;
    }
    break;
  default:
    return -4;
  }
  tvkprintf(net_connections, 4, "Processed nonce packet, crypto flags = %d\n", D->crypto_flags);
  return 0;
}

static int tcp_rpcc_send_handshake_packet (struct connection *c) {
  tvkprintf(net_connections, 4, "tcp_rpcc_send_handshake_packet\n");
  struct tcp_rpc_data *D = TCP_RPC_DATA (c);
  struct tcp_rpc_handshake_packet P{};
  if (!PID.ip) {
    init_client_PID(c->local_endpoint.ss_family == AF_INET ? inet_sockaddr_address(&c->local_endpoint): 0);
    if (!PID.ip) {
      PID.ip = get_my_ipv4 ();
    }
  }
  P.type = RPC_HANDSHAKE;
  P.flags = default_rpc_flags & RPC_CRYPTO_USE_CRC32C;
  if (!D->remote_pid.port) {
    const uint32_t remote_ip = inet_sockaddr_address(&c->remote_endpoint);
    D->remote_pid.ip = (remote_ip == LOCALHOST_NETWORK ? 0 : remote_ip);
    D->remote_pid.port = inet_sockaddr_port(&c->remote_endpoint);
  }
  memcpy (&P.sender_pid, &PID, sizeof (struct process_id));
  memcpy (&P.peer_pid, &D->remote_pid, sizeof (struct process_id));
  
  tcp_rpc_conn_send_data (c, sizeof (P), &P);
  TCP_RPCC_FUNC(c)->flush_packet (c);

  return 0;
}

static int tcp_rpcc_send_handshake_error_packet (struct connection *c, int error_code) {
  struct tcp_rpc_handshake_error_packet P{};
  if (!PID.pid) {
    init_client_PID(inet_sockaddr_address(&c->local_endpoint));
  }
  P.type = RPC_HANDSHAKE_ERROR;
  P.error_code = error_code;
  memcpy (&P.sender_pid, &PID, sizeof (PID));
  tcp_rpc_conn_send_data (c, sizeof (P), &P);
  TCP_RPCC_FUNC(c)->flush_packet (c);

  return 0;
}

static int tcp_rpcc_process_handshake_packet (struct connection *c, raw_message_t *msg) {
  tvkprintf(net_connections, 4, "tcp_rpcc_process_handshake_packet\n");

  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  struct tcp_rpc_handshake_packet P{};
  if (D->packet_num != -1 || D->packet_type != RPC_HANDSHAKE) {
    return -2;
  }
  if (D->packet_len != sizeof (P)) {
    tcp_rpcc_send_handshake_error_packet (c, -3);
    return -3;
  }
  assert (rwm_fetch_data (msg, &P, D->packet_len) == D->packet_len);  
  if (matches_pid (&P.sender_pid, &D->remote_pid) == no_pid_match) {
    tcp_rpcc_send_handshake_error_packet (c, -6);
    return -6;
  }
  if (!P.sender_pid.ip) {
    P.sender_pid.ip = D->remote_pid.ip;
  }
  memcpy (&D->remote_pid, &P.sender_pid, sizeof (struct process_id));
  if (matches_pid (&PID, &P.peer_pid) == no_pid_match) {
    tcp_rpcc_send_handshake_error_packet (c, -4);
    return -4;
  }
  if (P.flags & 0xff) {
    tcp_rpcc_send_handshake_error_packet (c, -7);
    return -7;
  }
  if (P.flags & RPC_CRYPTO_USE_CRC32C) {
    if (!(default_rpc_flags & RPC_CRYPTO_USE_CRC32C)) {
      tcp_rpcc_send_handshake_error_packet (c, -8);
      return -8;
    }
    D->crypto_flags |= RPC_CRYPTO_USE_CRC32C;
    D->custom_crc_partial = crc32c_partial;
  }
  return 0;
}

int tcp_rpcc_parse_execute (struct connection *c) {
  tvkprintf (net_connections, 4, "%s. in_total_bytes = %d\n", __func__, c->in.total_bytes);
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  int len;

  while (true) {
    len = c->in.total_bytes;
    if (len <= 0) {
      break;
    }
    if (!D->packet_len) {
      if (len < D->packet_v1_padding + 4) {
        c->status = conn_reading_answer;
        return D->packet_v1_padding + 4 - len;
      }
      if (D->packet_v1_padding) {
        assert(D->packet_v1_padding < 4);
        assert(rwm_fetch_data(&c->in, 0, D->packet_v1_padding) == D->packet_v1_padding);
        D->packet_v1_padding = 0;
      }
      assert (rwm_fetch_lookup (&c->in, &D->packet_len, 4) == 4);
      // We skip checks for len&3 == 0 for protocol version 0, because there is little value in it.
      if (D->packet_len > TCP_RPCC_FUNC(c)->max_packet_len && TCP_RPCC_FUNC(c)->max_packet_len > 0) {
        tvkprintf(net_connections, 1, "error while parsing packet: bad packet length %d\n", D->packet_len);
        c->status = conn_error;
        c->error = -1;
        return 0;
      }
    }
    if (D->packet_len == 4) {
      assert (rwm_fetch_data (&c->in, 0, 4) == 4);
      D->packet_len = 0;
      continue;
    }
    if (D->packet_len < 16) {
      tvkprintf(net_connections, 1, "error while parsing packet: bad packet length %d\n", D->packet_len);
      c->status = conn_error;
      c->error = -1;
      return 0;
    }
    if (len < D->packet_len) {
      c->status = conn_reading_answer;
      return D->packet_len - len;
    }

    raw_message_t msg;
    if (c->in.total_bytes == D->packet_len) {
      msg = c->in;
      rwm_init (&c->in, 0);
    } else {
      rwm_split_head (&msg, &c->in, D->packet_len);
    }

    unsigned crc32;
    assert (rwm_fetch_data_back (&msg, &crc32, 4) == 4);
    D->packet_crc32 = rwm_custom_crc32 (&msg, D->packet_len - 4, D->custom_crc_partial);
    if (crc32 != D->packet_crc32) {
      tvkprintf(net_connections, 1, "error while parsing packet: crc32 = %08x != %08x\n", D->packet_crc32, crc32);
      c->status = conn_error;
      c->error = -1;
      rwm_free (&msg);
      return 0;
    }

    assert (rwm_fetch_data (&msg, 0, 4) == 4);
    assert (rwm_fetch_data (&msg, &D->packet_num, 4) == 4);
    assert (rwm_fetch_lookup (&msg, &D->packet_type, 4) == 4);
    D->packet_len -= 12;

    tvkprintf(net_connections, 4, "received packet from connection %d\n", c->fd);
//    rwm_dump (&msg);

    int res = -1;

    if (D->packet_num != D->in_packet_num) {
      tvkprintf(net_connections, 1, "error while parsing packet: got packet num %d, expected %d\n", D->packet_num, D->in_packet_num);
      c->status = conn_error;
      c->error = -1;
      rwm_free (&msg);
      return 0;
    } else if (D->packet_num < 0) {
      /* this is for us */
      if (D->packet_num == -2) {
        c->status = conn_running;
        res = tcp_rpcc_process_nonce_packet (c, &msg);
        if (res >= 0) {
          res = tcp_rpcc_send_handshake_packet (c);
          //fprintf (stderr, "send_handshake_packet returned %d\n", res);
        }
      } else if (D->packet_num == -1) {
        c->status = conn_running;
        res = tcp_rpcc_process_handshake_packet (c, &msg);
        if (res >= 0 && TCP_RPCC_FUNC(c)->rpc_ready) {
          res = TCP_RPCC_FUNC(c)->rpc_ready (c);
        }
      }
      rwm_free (&msg);
      if (res < 0) {
        c->status = conn_error;
        c->error = res;
        return 0;
      }
    } else {
      /* main case */
      c->status = conn_running;
      if (D->packet_type == TL_RPC_PING) {
        res = tcp_rpcc_default_execute (c, D->packet_type, &msg);
      } else {
        res = TCP_RPCC_FUNC(c)->execute (c, D->packet_type, &msg);
      }
      if (res <= 0) {
        rwm_free (&msg);
      }
    }

    if (c->status == conn_error) {
      if (!c->error) {
        c->error = -2;
      }
      return 0;
    }
    
    D->in_packet_num++;
    if (c->crypto) {
      D->packet_v1_padding = (-D->packet_len) & 3;
    }
    D->packet_len = 0;
    if (c->status == conn_running) {
      c->status = conn_wait_answer;
    }
    if (c->status != conn_wait_answer) {
      break;
    }
  }
  return 0;
}

int tcp_check_perm (struct connection *c, int new_crypto_flags) {
  if (TCP_RPCC_FUNC(c)->rpc_check_perm) {
    int res = TCP_RPCC_FUNC(c)->rpc_check_perm(c);
    if (res < 0) {
      return res;
    }

    res &= 3;
    if (res == 0) {
      if (!aes_initialized) {
        kprintf("Crypto is required to connect to `%s`, but it is not initialized (see `aes-pwd` option)\n", sockaddr_storage_to_string(&c->remote_endpoint));
      }
      return -1;
    }

    TCP_RPC_DATA(c)->crypto_flags = res;
  } else {
    TCP_RPC_DATA(c)->crypto_flags = new_crypto_flags;
  }

  return 0;
}

int tcp_rpcc_connected (struct connection *c) {
  TCP_RPC_DATA(c)->out_packet_num = -2;
  c->last_query_sent_time = precise_now;

  int checked_perm = tcp_check_perm(c, 3);
  if (checked_perm < 0) {
    return checked_perm;
  }

  char buffer_local[SOCKADDR_STORAGE_BUFFER_SIZE], buffer_remote[SOCKADDR_STORAGE_BUFFER_SIZE];
  tvkprintf(net_connections, 4, "RPC connection #%d: %s -> %s connected, crypto_flags = %d\n", c->fd, sockaddr_storage_to_buffer(&c->local_endpoint, buffer_local),
           sockaddr_storage_to_buffer(&c->remote_endpoint, buffer_remote), TCP_RPC_DATA(c)->crypto_flags);

  assert (TCP_RPCC_FUNC(c)->rpc_init_crypto);
  int res = TCP_RPCC_FUNC(c)->rpc_init_crypto (c);

  if (res > 0) {
    assert (TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_NONCE_SENT);
  } else {
    return -1;
  }

  assert (TCP_RPCC_FUNC(c)->flush_packet);
  TCP_RPCC_FUNC(c)->flush_packet (c);

  return 0;
}


int tcp_rpcc_close_connection (struct connection *c, int who) {
  if (TCP_RPCC_FUNC(c)->rpc_close != NULL) {
    TCP_RPCC_FUNC(c)->rpc_close (c, who);
  }

  return client_close_connection (c, who);
}


int tcp_rpc_client_check_ready (struct connection *c) {
  return TCP_RPCC_FUNC(c)->check_ready (c);
}


int tcp_rpcc_init_fake_crypto (struct connection *c) {
  if (!(TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED)) {
    return -1;
  }

  struct tcp_rpc_nonce_packet buf{};
  buf.type = RPC_NONCE;
  buf.crypto_schema = RPC_CRYPTO_NONE;
  buf.protocol_version = 1; // ask for latest version we support

  tcp_rpc_conn_send_data (c, sizeof (buf), &buf);

  assert ((TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ENCRYPTED_MASK) == 0);
  TCP_RPC_DATA(c)->crypto_flags |= RPC_CRYPTO_NONCE_SENT;
 
  return 1;
}


int tcp_rpcc_init_outbound (struct connection *c) {
  tvkprintf(net_connections, 4, "rpcc_init_outbound (%d)\n", c->fd);
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  c->last_query_sent_time = precise_now;
  D->custom_crc_partial = crc32_partial;

  int checked_perm = tcp_check_perm(c, 1);
  if (checked_perm < 0) {
    return checked_perm;
  }

  D->in_packet_num = -2;

  return 0;
}

int tcp_rpcc_default_check_perm (struct connection *c) {
  int res = 0;
  if (aes_initialized > 0) {
    res |= RPC_CRYPTO_ALLOW_ENCRYPTED;
  }
  if (is_same_data_center(c, true)) {
    res |= RPC_CRYPTO_ALLOW_UNENCRYPTED;
  }
  return res;
}

int tcp_rpcc_default_check_perm_crypto (struct connection *c) {
  return tcp_rpcc_default_check_perm(c) & ~RPC_CRYPTO_ALLOW_UNENCRYPTED;
}



int tcp_rpcc_init_crypto (struct connection *c) {
  if (!(TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ALLOW_ENCRYPTED)) {
    return tcp_rpcc_init_fake_crypto (c);
  }

  TCP_RPC_DATA(c)->nonce_time = time (0);

  aes_generate_nonce (TCP_RPC_DATA(c)->nonce);

  struct tcp_rpc_nonce_packet buf{};
  memcpy (buf.crypto_nonce, TCP_RPC_DATA(c)->nonce, 16);
  buf.crypto_ts = TCP_RPC_DATA(c)->nonce_time;
  buf.type = RPC_NONCE;
  buf.key_select = get_crypto_key_id (default_aes_key);
  buf.crypto_schema = (TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) ? RPC_CRYPTO_NONE_OR_AES : RPC_CRYPTO_AES;
  buf.protocol_version = 1; // ask for latest version we support

  tcp_rpc_conn_send_data (c, sizeof (buf), &buf);

  assert ((TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ENCRYPTED_MASK) == RPC_CRYPTO_ALLOW_ENCRYPTED);
  TCP_RPC_DATA(c)->crypto_flags |= RPC_CRYPTO_NONCE_SENT;

  assert (!c->crypto);
  return 1;
}

int tcp_rpcc_start_crypto (struct connection *c, struct tcp_rpc_nonce_packet *P) {
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  tvkprintf(net_connections, 4, "rpcc_start_crypto: key_select = %d\n", P->key_select);

  if (c->crypto) {
    return -1;
  }

  if (c->in.total_bytes || c->out.total_bytes || !D->nonce_time) {
    return -1;
  }

  if (!P->key_select || P->key_select != get_crypto_key_id (default_aes_key)) {
    return -1;
  }

  struct aes_session_key aes_keys;

  if (aes_create_connection_keys (P->protocol_version, default_aes_key, &aes_keys, 1, P->crypto_nonce, D->nonce, P->crypto_ts, D->nonce_time, c) < 0) {
    return -1;
  }

  if (aes_crypto_init (c, &aes_keys, sizeof (aes_keys)) < 0) {
    return -1;
  }

  return 1;
}

void tcp_rpcc_flush_crypto (struct connection *c) {
  if (c->crypto) {
    int pad_bytes = c->type->crypto_needed_output_bytes (c);
    tvkprintf(net_connections, 4, "rpcc_flush_packet: padding with %d bytes\n", pad_bytes);
    if (pad_bytes > 0) {
      assert (!(pad_bytes & 3));
      int pad_str[3] = {4, 4, 4};
      assert (pad_bytes <= 12);
      assert (rwm_push_data (&c->out, pad_str, pad_bytes) == pad_bytes);
    }
  }
}

int tcp_rpcc_flush_packet (struct connection *c) {
  tcp_rpcc_flush_crypto (c);
  return flush_connection_output (c);
}

int tcp_rpcc_flush_packet_later (struct connection *c) {
  tcp_rpcc_flush_crypto (c);
  return flush_later (c);
}

int tcp_rpcc_flush (struct connection *c) {
  if (c->crypto) {
    int pad_bytes = c->type->crypto_needed_output_bytes (c);
    tvkprintf(net_connections, 4, "rpcs_flush: padding with %d bytes\n", pad_bytes);
    if (pad_bytes > 0) {
      assert (!(pad_bytes & 3));
      int pad_str[3] = {4, 4, 4};
      assert (pad_bytes <= 12);
      assert (rwm_push_data (&c->out, pad_str, pad_bytes) == pad_bytes);
    }
    return pad_bytes;
  }
  return 0;
}

int default_tcp_rpc_client_check_ready(struct connection *c) {
  server_check_ready(c);
  if (c->ready == cr_ok && TCP_RPC_DATA(c)->in_packet_num < 0) {
    if (TCP_RPC_DATA(c)->in_packet_num != -1 || c->status != conn_running) {
      c->ready = cr_notyet;
    }
  }
  return c->ready;
}
