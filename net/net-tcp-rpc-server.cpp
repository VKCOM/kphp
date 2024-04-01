// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-tcp-rpc-server.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "common/crc32.h"
#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/precise-time.h"
#include "common/tl/constants/common.h"

#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-events.h"
#include "net/net-ifnet.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-tcp-connections.h"

/*
 *
 *                BASIC RPC SERVER INTERFACE
 *
 */

int tcp_rpcs_alarm(struct connection *c);
int tcp_rpcs_close_connection(struct connection *c, int who);
int tcp_rpcs_default_check_perm(struct connection *c);
int tcp_rpcs_do_wakeup(struct connection *c);
int tcp_rpcs_flush(struct connection *c);
int tcp_rpcs_flush_packet(struct connection *c);
int tcp_rpcs_init_accepted(struct connection *c);
int tcp_rpcs_init_accepted_nohs(struct connection *c);
int tcp_rpcs_init_crypto(struct connection *c, struct tcp_rpc_nonce_packet *P);
int tcp_rpcs_parse_execute(struct connection *c);
int tcp_rpcs_wakeup(struct connection *c);

conn_type_t ct_tcp_rpc_server = {
  .magic = CONN_FUNC_MAGIC,
  .flags = C_RAWMSG,
  .title = "rpc_tcp_server",
  .accept = accept_new_connections,
  .init_accepted = tcp_rpcs_init_accepted,
  .create_outbound = NULL,
  .run = server_read_write,
  .reader = tcp_server_reader,
  .writer = tcp_server_writer,
  .close = tcp_rpcs_close_connection,
  .free_buffers = tcp_free_connection_buffers,
  .parse_execute = tcp_rpcs_parse_execute,
  .init_outbound = server_failed,
  .connected = server_failed,
  .wakeup = tcp_rpcs_wakeup,
  .alarm = tcp_rpcs_alarm,
  .ready_to_write = NULL,
  .check_ready = NULL,
  .wakeup_aio = NULL,
  .data_received = NULL,
  .data_sent = NULL,
  .ancillary_data_received = ancillary_data_received,
  .flush = tcp_rpcs_flush,
  .crypto_init = aes_crypto_init,
  .crypto_free = aes_crypto_free,
  .crypto_encrypt_output = tcp_aes_crypto_encrypt_output,
  .crypto_decrypt_input = tcp_aes_crypto_decrypt_input,
  .crypto_needed_output_bytes = tcp_aes_crypto_needed_output_bytes,
};

int tcp_rpcs_default_execute(struct connection *c, int op, raw_message_t *msg);

struct tcp_rpc_server_functions default_tcp_rpc_server = {.info = NULL,
                                                          .execute = tcp_rpcs_default_execute,
                                                          .check_ready = server_check_ready,
                                                          .flush_packet = tcp_rpcs_flush_packet,
                                                          .rpc_check_perm = tcp_rpcs_default_check_perm,
                                                          .rpc_init_crypto = tcp_rpcs_init_crypto,
                                                          .nop = NULL,
                                                          .rpc_wakeup = tcp_rpcs_do_wakeup,
                                                          .rpc_alarm = tcp_rpcs_do_wakeup,
                                                          .rpc_ready = server_noop,
                                                          .rpc_close = NULL,
                                                          .max_packet_len = 0,
                                                          .mode_flags = 0,
                                                          .memcache_fallback_type = NULL,
                                                          .memcache_fallback_extra = NULL,
                                                          .http_fallback_type = NULL,
                                                          .http_fallback_extra = NULL};

int tcp_rpcs_default_execute(struct connection *c, int op, raw_message_t *raw) {
  tvkprintf(net_connections, 4, "rpcs_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);
  if (op == TL_RPC_PING && raw->total_bytes == 12) {
    c->last_response_time = precise_now;
    static int Q[12];
    assert(rwm_fetch_data(raw, Q, 12) == 12);
    static int P[12];
    P[0] = TL_RPC_PONG;
    P[1] = Q[1];
    P[2] = Q[2];

    tvkprintf(net_connections, 4, "received ping from %s (val = %lld)\n", sockaddr_storage_to_string(&c->remote_endpoint), *(long long *)(P + 1));

    tcp_rpc_conn_send_data(c, 12, P);
    flush_later(c);
    return 0;
  }
  return 0;
}

static int tcp_rpcs_process_nonce_packet(struct connection *c, raw_message_t *msg) {
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  static struct tcp_rpc_nonce_packet P;
  int res;

  if (D->packet_num != -2 || D->packet_type != RPC_NONCE) {
    return -2;
  }
  if (D->packet_len != sizeof(struct tcp_rpc_nonce_packet)) {
    return -3;
  }

  assert(rwm_fetch_data(msg, &P, D->packet_len) == D->packet_len);
  int crypto_schema = P.crypto_schema;
  if (crypto_schema == RPC_CRYPTO_NONE_OR_AES) {
    if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
      crypto_schema = RPC_CRYPTO_NONE;
    } else {
      crypto_schema = RPC_CRYPTO_AES;
    }
  }
  switch (crypto_schema) {
    case RPC_CRYPTO_NONE: {
      if (P.crypto_schema == RPC_CRYPTO_NONE && P.key_select) {
        return -3;
      }
      if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
        D->crypto_flags = RPC_CRYPTO_ALLOW_UNENCRYPTED;
      } else {
        return -5;
      }
      break;
    }
    case RPC_CRYPTO_AES: {
      if (!P.key_select || P.key_select != get_crypto_key_id(default_aes_key)) {
        if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
          D->crypto_flags = RPC_CRYPTO_ALLOW_UNENCRYPTED;
          break;
        }
        return -3;
      }
      if (!(D->crypto_flags & RPC_CRYPTO_ALLOW_ENCRYPTED)) {
        if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
          D->crypto_flags = RPC_CRYPTO_ALLOW_UNENCRYPTED;
          break;
        }
        return -5;
      }
      D->nonce_time = time(0);
      if (abs(P.crypto_ts - D->nonce_time) > 30) {
        return -6; // less'om
      }
      D->crypto_flags &= ~RPC_CRYPTO_ALLOW_UNENCRYPTED;
      break;
    }
    default: {
      if (D->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED) {
        D->crypto_flags = RPC_CRYPTO_ALLOW_UNENCRYPTED;
        break;
      }
      return -4;
    }
  }
  res = TCP_RPCS_FUNC(c)->rpc_init_crypto(c, &P);
  if (res < 0) {
    return -6;
  }
  return 0;
}

static int tcp_rpcs_send_handshake_packet(struct connection *c) {
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  static struct tcp_rpc_handshake_packet P;
  assert(PID.pid);
  memset(&P, 0, sizeof(P));
  P.type = RPC_HANDSHAKE;
  P.flags = D->crypto_flags & RPC_CRYPTO_USE_CRC32C;
  memcpy(&P.sender_pid, &PID, sizeof(struct process_id));
  memcpy(&P.peer_pid, &D->remote_pid, sizeof(struct process_id));

  tcp_rpc_conn_send_data(c, sizeof(P), &P);

  TCP_RPCS_FUNC(c)->flush_packet(c);
  return 0;
}

static int tcp_rpcs_send_handshake_error_packet(struct connection *c, int error_code) {
  static struct tcp_rpc_handshake_error_packet P;
  assert(PID.pid);
  memset(&P, 0, sizeof(P));
  P.type = RPC_HANDSHAKE_ERROR;
  P.error_code = error_code;
  memcpy(&P.sender_pid, &PID, sizeof(PID));

  tcp_rpc_conn_send_data(c, sizeof(P), &P);

  TCP_RPCS_FUNC(c)->flush_packet(c);
  return 0;
}

static int tcp_rpcs_process_handshake_packet(struct connection *c, raw_message_t *msg) {
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  static struct tcp_rpc_handshake_packet P;
  if (!PID.ip) {
    init_server_PID(inet_sockaddr_address(&c->local_endpoint), inet_sockaddr_port(&c->local_endpoint));
    if (!PID.ip) {
      PID.ip = get_my_ipv4();
    }
  }
  if (D->packet_num != -1 || D->packet_type != RPC_HANDSHAKE) {
    return -2;
  }
  if (D->packet_len != sizeof(struct tcp_rpc_handshake_packet)) {
    tcp_rpcs_send_handshake_error_packet(c, -3);
    return -3;
  }
  assert(rwm_fetch_data(msg, &P, D->packet_len) == D->packet_len);
  memcpy(&D->remote_pid, &P.sender_pid, sizeof(struct process_id));
  if (matches_pid(&PID, &P.peer_pid) == no_pid_match) {
    tvkprintf(net_connections, 1, "PID mismatch during handshake: local %08x:%hu:%hu:%u, remote %08x:%hu:%hu:%u\n", PID.ip, PID.port, PID.pid, PID.utime,
              P.peer_pid.ip, P.peer_pid.port, P.peer_pid.pid, P.peer_pid.utime);
    tcp_rpcs_send_handshake_error_packet(c, -4);
    return -4;
  }
  if (P.flags & 0xff) {
    tcp_rpcs_send_handshake_error_packet(c, -7);
    return -7;
  }
  if (P.flags & default_rpc_flags & RPC_CRYPTO_USE_CRC32C) {
    D->crypto_flags |= RPC_CRYPTO_USE_CRC32C;
  }
  return 0;
}

static int __raw_msg_to_conn(void *extra, const void *data, int len) {
  connection *c = static_cast<connection *>(extra);
  assert(write_out(&c->In, data, len) == len);
  return 0;
}

int tcp_rpcs_parse_execute(struct connection *c) {
  tvkprintf(net_connections, 4, "%s. in_total_bytes = %d\n", __func__, c->in.total_bytes);
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);
  int len;

  while (true) {
    len = c->in.total_bytes;
    if (len <= 0) {
      break;
    }
    // fprintf (stderr, "in while : packet_len=%d, total_ready_bytes=%d; cptr=%p; c->status=%d\n", D->packet_len, len, c->Q.cptr, c->status);
    if (!D->packet_len) {
      if (len < 4) {
        c->status = conn_reading_query;
        return 4 - len;
      }
      assert(rwm_fetch_lookup(&c->in, &D->packet_len, 4) == 4);
      if (D->crypto_flags & 512) {
        D->flags = (D->flags & 0x7fffffff) | (D->packet_len & 0x80000000);
        D->packet_len &= 0x7fffffff;
      }
      if ((D->packet_len > TCP_RPCS_FUNC(c)->max_packet_len && TCP_RPCS_FUNC(c)->max_packet_len > 0)) {
        tvkprintf(net_connections, 1, "error while parsing packet: bad packet length %d\n", D->packet_len);
        c->status = conn_error;
        c->error = -1;
        return 0;
      }
      if (D->packet_len <= 0 || (D->packet_len & 0xc0000003)) {
        if (D->in_packet_num <= -2
            && (D->packet_len == 0x656c6564 || D->packet_len == 0x74617473 || D->packet_len == 0x73726576 || D->packet_len == 0x20746567
                || D->packet_len == 0x20746573 || D->packet_len == 0x20646461 || D->packet_len == 0x6c706572 || D->packet_len == 0x72636e69
                || D->packet_len == 0x72636564)
            && TCP_RPCS_FUNC(c)->memcache_fallback_type) {
          tvkprintf(net_connections, 4, "switching to memcache fallback for connection %d\n", c->fd);
          memset(c->custom_data, 0, sizeof(c->custom_data));
          c->type = static_cast<conn_type_t *>(TCP_RPCS_FUNC(c)->memcache_fallback_type);
          c->extra = TCP_RPCS_FUNC(c)->memcache_fallback_extra;

          assert(!c->out.total_bytes && !c->out_p.total_bytes && !c->in_u.total_bytes);
          rwm_free(&c->out);
          rwm_free(&c->out_p);
          rwm_free(&c->in_u);
          c->flags &= ~C_RAWMSG;

          init_builtin_buffer(&c->In, c->in_buff, BUFF_SIZE);
          init_builtin_buffer(&c->Out, c->out_buff, BUFF_SIZE);

          rwm_process_callback_t cb = __raw_msg_to_conn;
          rwm_process(&c->in, c->in.total_bytes, cb, c);
          rwm_free(&c->in);
          if (c->type->init_accepted(c) < 0) {
            tvkprintf(net_connections, 1, "memcache init_accepted() returns error for connection %d\n", c->fd);
            c->status = conn_error;
            c->error = -33;
            return 0;
          }
          nbit_set(&c->Q, &c->In);
          return c->type->parse_execute(c);
        }
        if (D->in_packet_num <= -2 && (D->packet_len == 0x44414548 || D->packet_len == 0x54534f50 || D->packet_len == 0x20544547)
            && TCP_RPCS_FUNC(c)->http_fallback_type) {
          tvkprintf(net_connections, 4, "switching to http fallback for connection %d\n", c->fd);
          memset(c->custom_data, 0, sizeof(c->custom_data));
          c->type = static_cast<conn_type_t *>(TCP_RPCS_FUNC(c)->http_fallback_type);
          c->extra = TCP_RPCS_FUNC(c)->http_fallback_extra;

          assert(!c->out.total_bytes && !c->out_p.total_bytes && !c->in_u.total_bytes);
          rwm_free(&c->out);
          rwm_free(&c->out_p);
          rwm_free(&c->in_u);
          c->flags &= ~C_RAWMSG;

          init_builtin_buffer(&c->In, c->in_buff, BUFF_SIZE);
          init_builtin_buffer(&c->Out, c->out_buff, BUFF_SIZE);

          rwm_process_callback_t cb = reinterpret_cast<rwm_process_callback_t>(__raw_msg_to_conn);
          rwm_process(&c->in, c->in.total_bytes, cb, c);
          rwm_free(&c->in);
          if (c->type->init_accepted(c) < 0) {
            tvkprintf(net_connections, 1, "http init_accepted() returns error for connection %d\n", c->fd);
            c->status = conn_error;
            c->error = -33;
            return 0;
          }
          nbit_set(&c->Q, &c->In);
          return c->type->parse_execute(c);
        }
        tvkprintf(net_connections, 1, "error while parsing packet: bad packet length %d\n", D->packet_len);
        c->status = conn_error;
        c->error = -1;
        return 0;
      }
    }
    if (D->packet_len == 4) {
      assert(rwm_fetch_data(&c->in, 0, 4) == 4);
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
      // fprintf (stderr, "need %d bytes, only %d present; need %d more\n", D->packet_len, len + 4, D->packet_len - len - 4);
      c->status = conn_reading_query;
      return D->packet_len - len;
    }

    raw_message_t msg;
    rwm_split_head(&msg, &c->in, D->packet_len);

    unsigned crc32;
    assert(rwm_fetch_data_back(&msg, &crc32, 4) == 4);
    D->packet_crc32 = rwm_custom_crc32(&msg, D->packet_len - 4, D->custom_crc_partial);
    if (crc32 != D->packet_crc32) {
      tvkprintf(net_connections, 1, "error while parsing packet: crc32 = %08x != %08x\n", D->packet_crc32, crc32);
      c->status = conn_error;
      c->error = -1;
      rwm_free(&msg);
      return 0;
    }

    assert(rwm_fetch_data(&msg, 0, 4) == 4);
    assert(rwm_fetch_data(&msg, &D->packet_num, 4) == 4);
    assert(rwm_fetch_lookup(&msg, &D->packet_type, 4) == 4);
    D->packet_len -= 12;

    if (verbosity > 2) {
      kprintf("received packet from connection %d (num %d)\n", c->fd, D->packet_num);
      rwm_dump(&msg);
    }

    int res = -1;

    if (D->in_packet_num == -3) {
      D->in_packet_num = 0;
    }

    if (!(D->crypto_flags & 256) && D->packet_num != D->in_packet_num) {
      tvkprintf(net_connections, 1, "error while parsing packet: got packet num %d, expected %d\n", D->packet_num, D->in_packet_num);
      c->status = conn_error;
      c->error = -1;
      rwm_free(&msg);
      return 0;
    } else if (D->packet_num < 0) {
      /* this is for us */
      if (D->packet_num == -2) {
        c->status = conn_running;
        res = tcp_rpcs_process_nonce_packet(c, &msg); // if res > 0, nonce packet sent in response
      } else if (D->packet_num == -1) {
        c->status = conn_running;
        res = tcp_rpcs_process_handshake_packet(c, &msg);
        if (res >= 0) {
          res = tcp_rpcs_send_handshake_packet(c);
          if (D->crypto_flags & RPC_CRYPTO_USE_CRC32C) {
            D->custom_crc_partial = crc32c_partial;
          }
          if (res >= 0 && TCP_RPCS_FUNC(c)->rpc_ready) {
            res = TCP_RPCS_FUNC(c)->rpc_ready(c);
          }
        }
      }
      rwm_free(&msg);
      if (res < 0) {
        c->status = conn_error;
        c->error = res;
        return 0;
      }
    } else {
      /* main case */
      c->status = conn_running;
      c->last_response_time = precise_now;
      if (D->packet_type == TL_RPC_PING) {
        res = tcp_rpcs_default_execute(c, D->packet_type, &msg);
      } else {
        res = TCP_RPCS_FUNC(c)->execute(c, D->packet_type, &msg);
      }
      if (res <= 0) {
        rwm_free(&msg);
      }
    }

    if (c->status == conn_error) {
      if (!c->error) {
        c->error = -2;
      }
      return 0;
    }

    D->in_packet_num++;

    if (c->status == conn_running) {
      c->status = conn_expect_query;
    }

    // assert ((c->pending_queries && (c->status == conn_wait_net || c->status == conn_wait_aio)) || (!c->pending_queries && c->status == conn_expect_query));
    assert(c->status == conn_wait_net || (c->pending_queries && c->status == conn_wait_aio) || (!c->pending_queries && c->status == conn_expect_query));

    D->packet_len = 0;
    if (c->status != conn_expect_query) {
      break;
    }
  }
  return 0;
}

int tcp_rpcs_wakeup(struct connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
    TCP_RPCS_FUNC(c)->rpc_wakeup(c);
  }
  if (c->out_p.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

int tcp_rpcs_alarm(struct connection *c) {
  TCP_RPCS_FUNC(c)->rpc_alarm(c);
  if (c->out_p.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

int tcp_rpcs_close_connection(struct connection *c, int who) {
  if (TCP_RPCS_FUNC(c)->rpc_close != NULL) {
    TCP_RPCS_FUNC(c)->rpc_close(c, who);
  }

  return server_close_connection(c, who);
}

int tcp_rpcs_do_wakeup(struct connection *c __attribute__((unused))) {
  return 0;
}

int tcp_rpcs_init_accepted(struct connection *c) {
  c->last_query_sent_time = precise_now;
  TCP_RPC_DATA(c)->custom_crc_partial = crc32_partial;

  if (TCP_RPCS_FUNC(c)->rpc_check_perm) {
    int res = TCP_RPCS_FUNC(c)->rpc_check_perm(c);
    tvkprintf(net_connections, 4, "rpcs_check_perm for connection %d: %s -> %s = %d\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint),
              sockaddr_storage_to_string(&c->local_endpoint), res);
    if (res < 0) {
      return res;
    }
    if (!(res &= 3)) {
      return -1;
    }

    TCP_RPC_DATA(c)->crypto_flags = res;
  } else {
    TCP_RPC_DATA(c)->crypto_flags = 1;
  }

  TCP_RPC_DATA(c)->in_packet_num = -2;
  TCP_RPC_DATA(c)->out_packet_num = -2;

  return 0;
}

int tcp_rpcs_init_accepted_nohs(struct connection *c) {
  TCP_RPC_DATA(c)->crypto_flags = 512 + 1;
  TCP_RPC_DATA(c)->in_packet_num = -3;
  TCP_RPC_DATA(c)->custom_crc_partial = crc32_partial;
  return TCP_RPCS_FUNC(c)->rpc_ready ? TCP_RPCS_FUNC(c)->rpc_ready(c) : 0;
}

int tcp_rpcs_init_fake_crypto(struct connection *c) {
  if (!(TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ALLOW_UNENCRYPTED)) {
    return -1;
  }

  static struct tcp_rpc_nonce_packet buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = RPC_NONCE;
  buf.crypto_schema = RPC_CRYPTO_NONE;

  tcp_rpc_conn_send_data(c, sizeof(buf), &buf);
  assert((TCP_RPC_DATA(c)->crypto_flags & RPC_CRYPTO_ENCRYPTED_MASK) == 0);
  TCP_RPC_DATA(c)->crypto_flags |= RPC_CRYPTO_NONCE_SENT;

  return 1;
}

int tcp_rpcs_default_check_perm(struct connection *c) {
  int res = 0;
  if (aes_initialized > 0) {
    res |= RPC_CRYPTO_ALLOW_ENCRYPTED;
  }
  if (is_same_data_center(c, false)) {
    res |= RPC_CRYPTO_ALLOW_UNENCRYPTED;
  }
  return res;
}

int tcp_rpcs_default_check_perm_crypted(struct connection *c) {
  return tcp_rpcs_default_check_perm(c) & ~RPC_CRYPTO_ALLOW_UNENCRYPTED;
}

int tcp_rpcs_default_check_perm_crypted_localhost(struct connection *c) {
  if (inet_sockaddr_address(&c->local_endpoint) == LOCALHOST && inet_sockaddr_address(&c->remote_endpoint) == LOCALHOST) {
    return RPC_CRYPTO_ALLOW_UNENCRYPTED | RPC_CRYPTO_ALLOW_ENCRYPTED;
  }
  return tcp_rpcs_default_check_perm(c) & ~RPC_CRYPTO_ALLOW_UNENCRYPTED;
}

int tcp_rpcs_init_crypto(struct connection *c, struct tcp_rpc_nonce_packet *P) {
  //  fprintf (stderr, "mcs_init_crypto (%p [fd=%d], '%.*s')\n", c, c->fd, key_len, key);
  struct tcp_rpc_data *D = TCP_RPC_DATA(c);

  if (c->crypto) {
    return -1;
  }

  if ((D->crypto_flags & (RPC_CRYPTO_ALLOW_ENCRYPTED | RPC_CRYPTO_ALLOW_UNENCRYPTED)) == RPC_CRYPTO_ALLOW_UNENCRYPTED) {
    return tcp_rpcs_init_fake_crypto(c);
  }

  if ((D->crypto_flags & (RPC_CRYPTO_ALLOW_ENCRYPTED | RPC_CRYPTO_ALLOW_UNENCRYPTED)) != RPC_CRYPTO_ALLOW_ENCRYPTED) {
    return -1;
  }

  if (c->In.total_bytes) {
    return -1;
  }

  aes_generate_nonce(D->nonce);

  struct aes_session_key aes_keys;

  if (aes_create_connection_keys(default_aes_key, &aes_keys, 0, D->nonce, P->crypto_nonce, P->crypto_ts, c) < 0) {
    return -1;
  }

  if (aes_crypto_init(c, &aes_keys, sizeof(aes_keys)) < 0) {
    return -1;
  }

  static struct tcp_rpc_nonce_packet buf;
  memset(&buf, 0, sizeof(buf));
  memcpy(buf.crypto_nonce, D->nonce, 16);
  buf.crypto_ts = D->nonce_time;
  buf.type = RPC_NONCE;
  buf.key_select = get_crypto_key_id(default_aes_key);
  buf.crypto_schema = RPC_CRYPTO_AES;

  tcp_rpc_conn_send_data(c, sizeof(buf), &buf);

  assert((D->crypto_flags & (RPC_CRYPTO_ALLOW_ENCRYPTED | RPC_CRYPTO_NONCE_SENT | RPC_CRYPTO_ENCRYPTION_ON)) == RPC_CRYPTO_ALLOW_ENCRYPTED);
  D->crypto_flags |= 4;

  assert(!c->out_p.total_bytes);
  raw_message_t x = c->out_p;
  c->out_p = c->out;
  c->out = x;

  assert(!c->in_u.total_bytes);
  x = c->in_u;
  c->in_u = c->in;
  c->in = x;
  return 1;
}

int tcp_rpcs_flush_packet(struct connection *c) {
  if (c->crypto) {
    int pad_bytes = c->type->crypto_needed_output_bytes(c);
    tvkprintf(net_connections, 4, "tcp_rpcs_flush_packet: padding with %d bytes\n", pad_bytes);
    if (pad_bytes > 0) {
      assert(!(pad_bytes & 3));
      static int pad_str[3] = {4, 4, 4};
      assert(pad_bytes <= 12);
      assert(rwm_push_data(&c->out, pad_str, pad_bytes) == pad_bytes);
    }
  }
  return flush_connection_output(c);
}

int tcp_rpcs_flush(struct connection *c) {
  if (c->crypto) {
    int pad_bytes = c->type->crypto_needed_output_bytes(c);
    tvkprintf(net_connections, 4, "rpcs_flush: padding with %d bytes\n", pad_bytes);
    if (pad_bytes > 0) {
      assert(!(pad_bytes & 3));
      static int pad_str[3] = {4, 4, 4};
      assert(pad_bytes <= 12);
      assert(rwm_push_data(&c->out, pad_str, pad_bytes) == pad_bytes);
    }
    return pad_bytes;
  }
  return 0;
}

/*
 *
 *                END (BASIC RPC SERVER)
 *
 */
