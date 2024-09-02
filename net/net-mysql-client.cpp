// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-mysql-client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"

#include "common/crypto/openssl-evp-digest.h"
#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-events.h"
#include "net/net-sockaddr-storage.h"

/*
 *
 *		MySQL CLIENT INTERFACE
 *
 */


static int sqlc_alarm (struct connection *c);
static int sqlc_wakeup (struct connection *c);
static int sqlc_connected (struct connection *c);
static int sqlc_check_ready (struct connection *c);
static int sqlc_ready_to_write (struct connection *c);

conn_type_t ct_mysql_client = {
  .magic = CONN_FUNC_MAGIC,
  .flags = 0,
  .title = "mysql_client",
  .accept = accept_new_connections,
  .init_accepted = server_failed,
  .create_outbound = NULL,
  .run = server_read_write,
  .reader = server_reader,
  .writer = server_writer,
  .close = client_close_connection,
  .free_buffers = free_connection_buffers,
  .parse_execute = sqlc_parse_execute,
  .init_outbound = sqlc_init_outbound,
  .connected = sqlc_connected,
  .wakeup = sqlc_wakeup,
  .alarm = sqlc_alarm,
  .ready_to_write = sqlc_ready_to_write,
  .check_ready = sqlc_check_ready,
  .wakeup_aio = NULL,
  .data_received = NULL,
  .data_sent = NULL,
  .ancillary_data_received = NULL,
  .flush = NULL,
  .crypto_init = aes_crypto_init,
  .crypto_free = aes_crypto_free,
  .crypto_encrypt_output = aes_crypto_encrypt_output,
  .crypto_decrypt_input = aes_crypto_decrypt_input,
  .crypto_needed_output_bytes = aes_crypto_needed_output_bytes,
};

enum sql_answer_parse_state {
  sqp_start,
  sqp_done
};

/*#pragma pack(push,1)

struct init_packet {
  int len;
  char proto_ver;
  char server_ver[8];
  struct mysql_auth_packet_end e;
};

#pragma pack(pop)

static struct init_packet P = {
  .len = sizeof (struct init_packet) - 4,
  .proto_ver = 10,
  .server_ver = "5.0.239",
  .e = {
    .server_capabilities = 0xa20c,
    .server_language = cp1251_general_ci,
    .server_status = 2
  }
};*/

#pragma pack(push,1)
struct client_auth_packet {
  int cli_flags;
  int max_packet_size;
  char charset_number;
  char filler[23];
};

#pragma pack(pop)

static struct client_auth_packet P = {
  .cli_flags = 0x3a685,
  .max_packet_size = 1 << 24,
  .charset_number = cp1251_general_ci,
  .filler = {0}
};

OPTION_PARSER(OPT_NETWORK, "sql-django-compat", no_argument, "compatibility mode to emulate django sql client with some strange flags") {
  P.cli_flags |= 2;
  return 0;
}

/*static int write_lcb (struct connection *c, unsigned long long l) {
  int res = 0;

  if (l <= 250) {
    res += write_out (&c->Out, (void *)&l, 1);
  } else 
  if (l <= 0xffff) {
    res += write_out (&c->Out, "\xfc", 1);
    res += write_out (&c->Out, (void *)&l, 2);
  } else
  if (l <= 0xffffff) {
    res += write_out (&c->Out, "\xfd", 1);
    res += write_out (&c->Out, (void *)&l, 3);
  } else {
    res += write_out (&c->Out, "\xfe", 1);
    res += write_out (&c->Out, (void *)&l, 8);
  }
  return res;
}*/


int sqlc_execute (struct connection *c, int op);
int sqlc_password (struct connection *c, const char *user, char buffer[20]);

struct mysql_client_functions default_mysql_client = {
  .info = NULL,
  .server_charset = cp1251_general_ci,
  .execute = sqlc_execute,
  .sql_authorized = server_noop,
  .sql_becomes_ready = server_noop,
  .sql_wakeup = sqlc_do_wakeup,
  .check_ready = NULL,
  .sql_flush_packet = sqlc_flush_packet,
  .sql_ready_to_write = NULL,
  .sql_check_perm = sqlc_default_check_perm,
  .sql_init_crypto = sqlc_init_crypto,
};

int sqlc_flush_packet (struct connection *c, int packet_len) {
  int pad_bytes = 0;
  if (c->crypto) {
    pad_bytes = c->type->crypto_needed_output_bytes (c);
    if (packet_len >= 0) {
      int b = SQLC_DATA(c)->block_size;
      packet_len += 4;
      packet_len = b - packet_len % b;
      if (packet_len == b) {
        packet_len = 0;
      }
      assert (packet_len == pad_bytes);
    }
    vkprintf(4, "sqlc_flush_query: padding with %d bytes\n", pad_bytes);
    if (pad_bytes > 0) {
      static char pad_str[16];
      assert (pad_bytes <= 15);
      memset (pad_str, pad_bytes, pad_bytes);
      write_out (&c->Out, pad_str, pad_bytes);
    }
  }
  return pad_bytes;
}




static int sqlc_inner_authorise (struct connection *c) {
  struct sqlc_data *D = SQLC_DATA(c);
  int len = D->packet_len, i;
  char *p, *q, *r;
  char password_sha1[48], stage1_hash[20], user_scramble[20];
  nbw_iterator_t it;
  struct mysql_auth_packet_end *T;
  char scramble_len = 20;

  vkprintf(4, "server_auth_packet received, len=%d\n", D->packet_len);
  if (len >= 0x800) {
    vkprintf(4, "server_auth_packet too large\n");
    return -1;
  }
  if (len < 46) {
    vkprintf(4, "server_auth_packet too small\n");
    return -1;
  }
  assert (force_ready_bytes (&c->In, len+4) >= len+4);
  p = ((char *)get_read_ptr (&c->In)) + 4;
  q = p + len;

  if (*p != 10) {
    vkprintf(4, "server_auth_packet has bad protocol version\n");
    return -1;
  }

  p++;

  r = p;
    
  while (p < q && *p) {
    p++;
  }

  if (p == q) {
    vkprintf(4, "unterminated version string in server_auth_packet\n");
    return -1;
  }

  if (p - r < 8) {
    memcpy (D->version, r, p - r);
  } else {
    memcpy (D->version, r, 8);
  }
    
  p++;

  size_t expected_length = sizeof (struct mysql_auth_packet_end);
  if (((struct mysql_auth_packet_end*)p)->proto_len){
    if (strncmp(((struct mysql_auth_packet_end*)p)->proto, "mysql_native_password", 21)){
      vkprintf(4, "unknown auth_proto %s\n", ((struct mysql_auth_packet_end*)p)->proto);
      return -1;
    }
    expected_length += 22;
  }

  if (q - p != expected_length) {
    vkprintf(4, "server_auth_packet has incorrect size\n");
    return -1;
  }

  int res = !SQLC_FUNC(c)->is_mysql_same_datacenter_check_disabled() && SQLC_FUNC(c)->sql_check_perm ? SQLC_FUNC(c)->sql_check_perm (c) : 1;

  if (res < 0 || !(res &= 3)) {
    vkprintf(4, "check_perm forbids access for connection %d\n", c->fd);
    return -1;
  }

  D->crypto_flags = res;

  vkprintf(4, "crypto flags here = %d\n", D->crypto_flags);

  if ((res & 2) && p - r >= 8 && !memcmp(r, "5.0.239-", 8) && SQLC_FUNC(c)->sql_init_crypto && SQLC_FUNC(c)->sql_init_crypto (c, r + 8, p - r - 9) > 0) {
    D->crypto_flags &= 2;
    D->crypto_flags |= 8;
  } else {
    D->crypto_flags &= 1;
  }

  vkprintf(4, "crypto flags adjusted %d\n", D->crypto_flags);

  if (!(D->crypto_flags & 3)) {
    vkprintf(4, "unable to initialise cryptography, closing connection %d\n", c->fd);
    return -1;
  }

  T = (struct mysql_auth_packet_end *)p;

  const char *sql_database = SQLC_FUNC(c)->sql_get_database(c);
  const char *sql_user = SQLC_FUNC(c)->sql_get_user(c);
  const char *sql_password = SQLC_FUNC(c)->sql_get_password(c);

  vkprintf(4, "mysql db: %s; user: %s; password: *\n", sql_database, sql_user);

  vk::sha1({(unsigned char *)sql_password, strlen(sql_password)}, {reinterpret_cast<unsigned char *>(stage1_hash), 5});
  memcpy (password_sha1, T->scramble1, 8);
  memcpy (password_sha1 + 8, T->scramble2, 12);
  vk::sha1({reinterpret_cast<unsigned char *>(stage1_hash), 20}, {reinterpret_cast<unsigned char *>(password_sha1 + 20), 5});
  vk::sha1({reinterpret_cast<unsigned char *>(password_sha1), 40}, {reinterpret_cast<unsigned char *>(user_scramble), 5});
  for (i = 0; i < 20; i++) {
    user_scramble[i] ^= stage1_hash[i];
  }

  nbit_setw (&it, &c->Out);
  unsigned temp = 0x01000000;
  write_out (&c->Out, &temp, 4);

  len = 0;
  len += write_out (&c->Out, &P, sizeof (P));
  len += write_out (&c->Out, sql_user, strlen (sql_user) + 1);
  len += write_out (&c->Out, &scramble_len, 1);
  len += write_out (&c->Out, user_scramble, 20);
  if (sql_database && *sql_database) {
    len += write_out (&c->Out, sql_database, strlen (sql_database) + 1);
  }
  nbit_write_out (&it, &len, 3);

  SQLC_FUNC(c)->sql_flush_packet (c, len);

  return 0;
}


int sqlc_parse_execute (struct connection *c) {
  struct sqlc_data *D = SQLC_DATA(c);
  int len = nbit_total_ready_bytes (&c->Q);
  static unsigned int psize;
  vkprintf(4, "sqlc_parse_execute(%d), status=%d, bytes=%d, packet_state=%d, packet_len=%d\n", c->fd, c->status, len, D->packet_state, D->packet_len);
  char *p;

  while (len > 0 && !(c->flags & (C_FAILED | C_STOPREAD))) {
    c->status = conn_reading_answer;
    if (D->packet_state == 0) {
      if (len < 4) {
        return 4 - len;
      }
      assert (nbit_read_in (&c->Q, &psize, 4) == 4);
      len -= 4;
      D->packet_state = 1;
      D->packet_len = psize & 0xffffff;
      D->packet_seq = (psize >> 24);
      if (D->block_size > 1) {
        D->packet_padding = D->block_size - (D->packet_len + 4) % D->block_size;
        if (D->packet_padding == D->block_size) {
          D->packet_padding = 0;
        }
      }
    }
    assert (D->packet_state == 1);
    if (len < D->packet_len + D->packet_padding) {
      return D->packet_len + D->packet_padding - len;
    }
    /* complete packet ready */
    c->last_response_time = precise_now;
    vkprintf(4, "client packet ready: len=%d, seq_num=%d\n", D->packet_len, D->packet_seq);
    if (D->auth_state == sql_noauth) {
      int res = sqlc_inner_authorise(c);
      if (res < 0) {
        c->status = conn_error;
        c->error = -1;
        return 0;
      }
      D->auth_state = sql_auth_sent;
      advance_skip_read_ptr (&c->In, D->packet_len + D->packet_padding + 4);
      if ((D->crypto_flags & 10) == 10) {
        mark_all_unprocessed (&c->In);
      }
      nbit_set (&c->Q, &c->In);
      len = nbit_ready_bytes (&c->Q);
    } else 
    if (D->auth_state == sql_auth_sent) {
      /* parse OK|failed */
      p = ((char *)get_read_ptr (&c->In)) + 4;
      if (D->packet_len == 0 || *p) {
        c->status = conn_error;
        c->error = -1;
        kprintf("ok packet expected in response to authentification from connection %d (%s)\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint));
        return 0;
      }
      D->auth_state = sql_auth_ok;
      advance_skip_read_ptr (&c->In, D->packet_len + D->packet_padding + 4);

      if (SQLC_FUNC(c)->sql_authorized (c)) {
        c->status = conn_error;
        c->error = -1;
        vkprintf(4, "sql authorisation failed\n");
        return 0;
      }

      vkprintf(4, "outcoming authorization successful\n");

    } else 
    if (D->auth_state == sql_auth_initdb) {
      p = ((char *)get_read_ptr (&c->In)) + 4;
      if (D->packet_len == 0 || *p) {
        c->status = conn_error;
        c->error = -1;
        vkprintf(4, "ok packet expected in response to initdb\n");
        return 0;
      }
      D->auth_state = sql_auth_ok;
      advance_skip_read_ptr (&c->In, D->packet_len + D->packet_padding + 4);
      c->status = conn_ready;
      D->packet_state = 0;
      /*if (SQLC_FUNC(c)->sql_authorized (c)) {
        c->status = conn_error;
        c->error = -1;
        kprintf ("ok packet expected\n");
        return 0;
      }*/
      vkprintf(4, "outcoming initdb successful\n");
      SQLC_FUNC(c)->sql_becomes_ready (c);
      return 0;
    } else {
      int op = (D->packet_len > 0 ? *(char *) nbit_get_ptr (&c->Q) : -1);

      assert (D->auth_state == sql_auth_ok);

      //dump_connection_buffers (c);
      vkprintf(4, "execute, op=%d\n", op);

      int keep_total_bytes = c->In.total_bytes;

      /* execute */
      c->status = conn_running;
      int res = SQLC_FUNC(c)->execute (c, op);

      //dump_connection_buffers (c);

      if (res == SKIP_ALL_BYTES) {
//      assert (keep_total_bytes == c->In.total_bytes);  // this assert FAILS!
        if (keep_total_bytes != c->In.total_bytes) {
          kprintf ("error: in SKIP_ALL_BYTES for connection %d: keep_total=%d != total_bytes=%d, packet_len=%d, packet_padding=%d, packet_state=%d, packet_seq=%d, op=%d, status=%d\n",
          		    c->fd, keep_total_bytes, c->In.total_bytes, D->packet_len, D->packet_padding, D->packet_state, D->packet_seq, op, c->status);
        }
        advance_skip_read_ptr (&c->In, D->packet_len + D->packet_padding + 4);
      } else if (keep_total_bytes != c->In.total_bytes) {
        assert (keep_total_bytes == c->In.total_bytes + D->packet_len + 4);
        advance_skip_read_ptr (&c->In, D->packet_padding);
      }
    }
    nbit_set (&c->Q, &c->In);
    len = nbit_ready_bytes (&c->Q);
    D->packet_state = 0;
    if (c->status == conn_running) {
      c->status = conn_wait_answer;
    }
  }

  return 0;
}

int sqlc_execute (struct connection *c __attribute__((unused)), int op __attribute__((unused))) {//TODO: this unused generated by script, recheck it.
//  struct sqlc_data *D = SQLC_DATA(c);
  //send_error_packet (c, 1045, 28000, "Failed", 6, 1);
  return SKIP_ALL_BYTES;
}


static int sqlc_wakeup (struct connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_wait_answer;
    SQLC_FUNC(c)->sql_wakeup (c);
  }
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

static int sqlc_alarm (struct connection *c) {
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

int sqlc_password (struct connection *c __attribute__((unused)), const char *user, char buffer[20]) {//TODO: this unused generated by script, recheck it.
  memset (buffer, 0, 20);

  if (!strcmp (user, "kitten")) {
    unsigned char buffer2[20];
    vk::sha1({(unsigned char *)"test", 4}, buffer2);
    vk::sha1({buffer2, 20}, {(unsigned char *)buffer, 20});
    return 2;
  }
  
  return 1;
}



static int sqlc_connected (struct connection *c) {
  c->status = conn_wait_answer;
  return 0;
}


int sqlc_do_wakeup (struct connection *c __attribute__((unused))) {//TODO: this unused generated by script, recheck it.
//  struct sqlc_data *D = SQLC_DATA(c);
//  if (D->query_type == sqlt_SELECT) {
    /* return DROP DATABASE to client */
//  }

  return 0;
}


int sqlc_check_ready (struct connection *c) {
  return SQLC_FUNC(c)->check_ready (c);
}

int sqlc_init_outbound (struct connection *c) {
  c->last_query_sent_time = precise_now;
  return 0;
}

static int sqlc_ready_to_write (struct connection *c) {
  if (SQLC_FUNC(c)->sql_ready_to_write) {
    SQLC_FUNC(c)->sql_ready_to_write (c);
  }
  return 0;
}

/*
 *
 *		CRYPTO
 *
 */


int sqlc_default_check_perm (struct connection *c) {
  if (aes_initialized <= 0) {
    return (!is_same_data_center(c, 1)) ? 0 : 1;
  }
  return (!is_same_data_center(c, 1)) ? 2 : 1;
}

int sqlc_init_crypto (struct connection *c, char *key, int key_len) {
  vkprintf(4, "sqlc_init_crypto (%p [fd=%d], '%.*s' [%d])\n", c, c->fd, key_len, key, key_len);

  if (c->crypto) {
    return -1;
  }
  if (key_len != 43 || key[0] != 'A' || key[1] != ':' || key[10] != ':' || key[43]) {
    return -1;
  }

  char *tmp;

  int utime = strtoul (key + 2, &tmp, 16);
  if (tmp != key + 10) {
    return -1;
  }

  int mytime = time (0);

  vkprintf(4, "remote time %d, local %d\n", utime, mytime);

  if (abs (mytime - utime) > 10) {
    return -1;
  }

  static char nonce_in[16], nonce_out[16];

  *(long long *)(nonce_in + 8) = strtoull (key + 27, &tmp, 16);
  if (tmp != key + 43) {
    return -1;
  }

  key[27] = 0;
  *(long long *)nonce_in = strtoull (key + 11, &tmp, 16);
  if (tmp != key + 27) {
    return -1;
  }

  aes_generate_nonce (nonce_out);

  struct aes_session_key aes_keys;

  if (aes_create_connection_keys (default_aes_key, &aes_keys, 1, nonce_in, nonce_out, utime, c) < 0) {
    return -1;
  }

  int v = 0xcc000010;

  if (aes_crypto_init (c, &aes_keys, sizeof (aes_keys)) < 0) {
    return -1;
  }

  write_out (&c->Out, &v, 4);
  write_out (&c->Out, nonce_out, 16);

  mark_all_processed (&c->Out);

  SQLC_DATA(c)->block_size = 16;

  return 1;
}

/*
 *
 *		END (MySQL CLIENT)
 *
 */

