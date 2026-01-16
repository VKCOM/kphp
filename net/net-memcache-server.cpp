// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-memcache-server.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/server/stats.h"
#include "common/stats/provider.h"

#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-events.h"
#include "net/net-sockaddr-storage.h"

#define MAX_KEY_LEN     1000
#define MAX_VALUE_LEN   1048576

static int mcs_wakeup (struct connection *c);

conn_type_t ct_memcache_server = {
  .magic = CONN_FUNC_MAGIC,
  .flags = 0,
  .title = "memcache_server",
  .accept = accept_new_connections,
  .init_accepted = mcs_init_accepted,
  .create_outbound = NULL,
  .run = server_read_write,
  .reader = server_reader,
  .writer = server_writer,
  .close = server_close_connection,
  .free_buffers = free_connection_buffers,
  .parse_execute = mcs_parse_execute,
  .init_outbound = server_failed,
  .connected = server_failed,
  .wakeup = mcs_wakeup,
  .alarm = mcs_alarm,
  .ready_to_write = NULL,
  .check_ready = NULL,
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

enum mc_query_parse_state {
  mqp_start,
  mqp_readcommand,
  mqp_skipspc,
  mqp_readkey,
  mqp_readint,
  mqp_readints,
  mqp_skiptoeoln,
  mqp_readtoeoln,
  mqp_eoln,
  mqp_wantlf,
  mqp_done
};

int mcs_execute (struct connection *c, int op) {
  struct mcs_data *D = MCS_DATA(c);
  static char key_buffer[MAX_KEY_LEN+4];
  int len, res, skip, keep_total_bytes;
  char *ptr, *ptr_e, *ptr_s, *keep_write_ptr;
  nb_iterator_t r_it;

  if (verbosity > 0 && (op != mct_empty || D->query_len != 1 || verbosity > 4)) {
    kprintf ("mc_server: op=%d, key_len=%d, arg#=%d, query_len=%d\n", op, D->key_len, D->arg_num, D->query_len);
  }

  if (op != mct_empty) {
    netw_queries++;
    if (op != mct_get && op != mct_get_resume && op != mct_stats && op != mct_version) {
      netw_update_queries++;
    }
  }

  switch (op) {

  case mct_empty:
    break;

  case mct_set:
  case mct_add:
  case mct_replace:

    if (D->key_len > 0 && D->key_len <= MAX_KEY_LEN && D->arg_num == 3 && (unsigned) D->args[2] <= MAX_VALUE_LEN) {
      int needed_bytes = D->args[2] + D->query_len + 2 - c->In.total_bytes;
      if (needed_bytes > 0) {
        return needed_bytes;
      }
      nbit_advance (&c->Q, D->args[2]);
      len = nbit_ready_bytes (&c->Q);
      assert (len > 0);
      ptr = static_cast<char*>(nbit_get_ptr (&c->Q));
    } else {
      D->query_flags |= 16;
      return SKIP_ALL_BYTES;
    }
    if (len == 1) {
      nbit_advance (&c->Q, 1);
    }
    if (ptr[0] != '\r' || (len > 1 ? ptr[1] : *((char *) nbit_get_ptr(&c->Q))) != '\n') {
      D->query_flags |= 16;
      return SKIP_ALL_BYTES;
    }
    assert (advance_skip_read_ptr (&c->In, D->key_offset) == D->key_offset);
    assert (read_in (&c->In, key_buffer, D->key_len) == D->key_len);
    key_buffer[D->key_len] = 0;
    skip = D->query_len - D->key_offset - D->key_len;
    assert (advance_skip_read_ptr (&c->In, skip) == skip);

    vkprintf(4, "mc: set op=%d, key '%s', key_len=%d, flags=%lld, time=%lld, value_len=%lld\n", op, key_buffer, D->key_len, D->args[0], D->args[1], D->args[2]);

  restart_set:

    len = 2;
    keep_total_bytes = c->In.total_bytes;

    res = MCS_FUNC(c)->mc_store (c, D->query_type, key_buffer, D->key_len, D->args[0], D->args[1], D->args[2]);
    if (res < -1) {
      len += D->args[2];
    }

    if (c->status == conn_wait_aio) {
      assert (!res);
      assert (c->In.total_bytes == keep_total_bytes);
      if (!c->Tmp) {
        c->Tmp = alloc_head_buffer();
        assert (c->Tmp);
        D->query_flags |= 64;
      }
      assert (write_out (c->Tmp, key_buffer, D->key_len) == D->key_len);
      D->query_type += mct_set_resume - mct_set;
      return 0;
    }

    assert (advance_skip_read_ptr (&c->In, len) == len);

    if (res > 0) {
      write_out (&c->Out, "STORED\r\n", 8);
    } else {
      write_out (&c->Out, "NOT_STORED\r\n", 12);
    }

    return 0;

  case mct_set_resume:
  case mct_add_resume:
  case mct_replace_resume:

    assert (c->Tmp);
    assert (c->Tmp->total_bytes >= D->key_len);
    assert (read_back (c->Tmp, key_buffer, D->key_len) == D->key_len);
    if (D->query_flags & 64) {
      assert (!c->Tmp->total_bytes);
      free_tmp_buffers (c);
      D->query_flags &= ~64;
    }
    key_buffer[D->key_len] = 0;
    D->query_type = op -= mct_set_resume - mct_set;

    vkprintf(4, "mc: set_resume op=%d, key '%s', key_len=%d, flags=%lld, time=%lld, value_len=%lld\n", op, key_buffer, D->key_len, D->args[0], D->args[1], D->args[2]);

    goto restart_set;

  case mct_get:
    assert (advance_skip_read_ptr (&c->In, 3) == 3);
    MCS_DATA(c)->get_count = 0;
    /* fallthrough */
  case mct_get_resume:
    nbit_set (&r_it, &c->In);
    while (true) {
      ptr = ptr_s = static_cast<char*>(nbit_get_ptr (&r_it));
      ptr_e = ptr + nbit_ready_bytes (&r_it);
      while (ptr < ptr_e && (*ptr == ' ' || *ptr == '\t')) {
        ptr++;
      }
      if (ptr == ptr_e) {
        nbit_advance (&r_it, ptr - ptr_s);
        advance_read_ptr (&c->In, ptr - ptr_s);
        continue;
      }
      if (*ptr == '\r' || *ptr == '\n') {
        break;
      }
      len = 0;
      skip = 0;
      while (ptr < ptr_e && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n') {
        if (len <= MAX_KEY_LEN) {
          key_buffer[len++] = *ptr;
        }
        ptr++;
        if (ptr == ptr_e) {
          nbit_advance (&r_it, ptr - ptr_s);
          skip += ptr - ptr_s;
          ptr = ptr_s = static_cast<char*>(nbit_get_ptr (&r_it));
          ptr_e = ptr + nbit_ready_bytes (&r_it);
        }
      }
      assert (ptr < ptr_e);
      if (len <= MAX_KEY_LEN) {
        key_buffer[len] = 0;
        if (!MCS_DATA(c)->get_count && op != mct_get_resume) {
          MCS_FUNC(c)->mc_get_start (c);
        }
        keep_write_ptr = get_write_ptr (&c->Out, 0);

        res = MCS_FUNC(c)->mc_get (c, key_buffer, len);

        if (c->status == conn_wait_aio) {
          advance_skip_read_ptr (&c->In, skip + ptr - ptr_s - len);
          MCS_DATA(c)->query_type = mct_get_resume;

          return 0;
        } else if (res < 0) {
          if (get_write_ptr (&c->Out, 0) == keep_write_ptr && !(c->flags & C_INTIMEOUT)) {
            return_one_key_false (c, key_buffer, len);
          }
        }
        MCS_DATA(c)->get_count++;
      }
      nbit_advance (&r_it, ptr - ptr_s);
      advance_skip_read_ptr (&c->In, skip + ptr - ptr_s);
    }
    assert (ptr < ptr_e && (*ptr == '\r' || *ptr == '\n'));
    ptr++;
    if (ptr < ptr_e && ptr[-1] == '\r' && ptr[0] == '\n') {
      ptr++;
    }
    advance_read_ptr (&c->In, ptr - ptr_s);

    if (MCS_DATA(c)->get_count) {
      MCS_FUNC(c)->mc_get_end (c, MCS_DATA(c)->get_count);
    } else {
      write_out (&c->Out, "END\r\n", 5);
    }

    return 0;

  case mct_incr:
  case mct_decr:
  case mct_delete:

    if (!(D->key_len > 0 && D->key_len <= MAX_KEY_LEN && (unsigned) D->arg_num <= 1 && (D->arg_num || op == mct_delete))) {
      D->query_flags |= 16;
      return SKIP_ALL_BYTES;
    }

    nbit_set (&r_it, &c->In);
    assert (nbit_advance (&r_it, D->key_offset) == D->key_offset);
    assert (nbit_read_in (&r_it, key_buffer, D->key_len) == D->key_len);
    key_buffer[D->key_len] = 0;

    if (op == mct_delete) {
      vkprintf(4, "mc_delete: key '%s', key_len=%d\\n", key_buffer, D->key_len);
      res = MCS_FUNC(c)->mc_delete (c, key_buffer, D->key_len);
    } else { 
      vkprintf(4, "mc_incr: op=%d, key '%s', key_len=%d, arg=%lld\n", op, key_buffer, D->key_len, D->args[0]);
      res = MCS_FUNC(c)->mc_incr (c, op - mct_incr, key_buffer, D->key_len, D->args[0]);
    }
    
    if (c->status != conn_wait_aio) {
      assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);
    }

    return 0;

  case mct_CLOSE:
    c->status = conn_error;
    c->error = -1;
    return 0;

  case mct_stats:

    if (D->key_len || D->arg_num) {
      D->query_flags |= 16;
      return SKIP_ALL_BYTES;
    }
    assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);

    res = MCS_FUNC(c)->mc_stats (c);
    
    return 0;

  case mct_version:

    if (D->key_len || D->arg_num) {
      D->query_flags |= 16;
      return SKIP_ALL_BYTES;
    }
    assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);

    res = MCS_FUNC(c)->mc_version (c);
    
    return 0;


  default:
    D->query_flags |= 16;
    break;
  }

  assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);
  return 0;

}

int mcs_parse_execute (struct connection *c) {
  struct mcs_data *D = MCS_DATA(c);
  char *ptr, *ptr_s, *ptr_e;
  int len;
  long long tt;

  vkprintf(4, "in mcs_execute c->pending_queries = %d, c->status = %d\n", c->pending_queries, c->status);
  while (c->status == conn_expect_query || c->status == conn_reading_query) {
    len = nbit_ready_bytes (&c->Q);
    ptr = ptr_s = static_cast<char*>(nbit_get_ptr (&c->Q));
    ptr_e = ptr + len;
    if (len <= 0) {
      break;
    }

    while (ptr < ptr_e && c->parse_state != mqp_done) {

      switch (c->parse_state) {
      case mqp_start:
        D->clen = 0;
        D->query_flags = 0;
        D->query_type = mct_none;
        D->query_len = 0;
        D->key_offset = 0;
        D->key_len = 0;
        D->arg_num = 0;
        c->parse_state = mqp_readcommand;
        /* fallthrough */
      case mqp_readcommand:
        if (!D->clen && *ptr == '~') {
          ptr++;
          D->query_flags = 1;
        }
        while (ptr < ptr_e && D->clen < 15 && ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z'))) {
          D->comm[D->clen++] = *ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        if (D->clen == 15 || (*ptr != '\t' && *ptr != ' ' && *ptr != '\r' && *ptr != '\n')) {
          c->parse_state = mqp_skiptoeoln;
          break;
        }
        D->comm[D->clen] = 0;
        switch (D->comm[0]) {
        case 'C':
          if (!strcmp (D->comm, "CLOSE")) {
            D->query_type = mct_CLOSE;
          }
          break;
        case 'Q':
          if (!strcmp (D->comm, "QUIT")) {
            D->query_type = mct_QUIT;
          }
          break;
        case 'v':
          if (!strcmp (D->comm, "version")) {
            D->query_type = mct_version;
          }
          break;
        case 's':
          if (!strcmp (D->comm, "set")) {
            D->query_type = mct_set;
          } else if (!strcmp (D->comm, "stats")) {
            D->query_type = mct_stats;
          }
          break;
        case 'r':
          if (!strcmp (D->comm, "replace")) {
            D->query_type = mct_replace;
          }
          break;
        case 'a':
          if (!strcmp (D->comm, "add")) {
            D->query_type = mct_add;
          }
          break;
        case 'i':
          if (!strcmp (D->comm, "incr")) {
            D->query_type = mct_incr;
          }
          break;
        case 'd':
          if (!strcmp (D->comm, "decr")) {
            D->query_type = mct_decr;
          } else if (!strcmp (D->comm, "delete")) {
            D->query_type = mct_delete;
          }
          break;
        case 'g':
          if (!strcmp (D->comm, "get")) {
            D->query_type = mct_get;
          }
          break;
        case 0:
          D->query_type = mct_empty;
        }

        if (D->query_type == mct_none) {
          c->parse_state = mqp_skiptoeoln;
          break;
        }

        D->key_offset = D->clen;
        c->parse_state = mqp_skipspc;
        /* fallthrough */

      case mqp_skipspc:

        while (ptr < ptr_e && (*ptr == ' ' || *ptr == '\t')) {
          D->key_offset++;
          ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        if (*ptr != '\r' && *ptr != '\n') {
          if (D->query_type >= mct_stats) {
            c->parse_state = mqp_skiptoeoln;
            break;
          } else if (D->query_type == mct_get) {
            c->parse_state = mqp_readtoeoln;
            break;
          } else {
            c->parse_state = mqp_readkey;
          }
        } else {
          c->parse_state = mqp_eoln;
          break;
        }
       
      case mqp_readkey:
        while (ptr < ptr_e && (unsigned char) *ptr > ' ') {
          D->key_len++;
          ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        if (*ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n') {
          c->parse_state = mqp_skiptoeoln;
          break;
        }
        D->arg_num = 0;
        c->parse_state = mqp_readints;
        /* fallthrough */

      case mqp_readints:

        while (ptr < ptr_e && (*ptr == ' ' || *ptr == '\t')) {
          ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        if (*ptr == '\r' || *ptr == '\n') {
          c->parse_state = mqp_eoln;
          break;
        }
        if (D->arg_num >= 4) {
          c->parse_state = mqp_skiptoeoln;
          break;
        }
        D->args[D->arg_num] = 0;
        if (*ptr == '-') {
          ptr++;
          D->query_flags |= 6;
        } else {
          D->query_flags |= 4;
        }
        c->parse_state = mqp_readint;
        /* fallthrough */
      case mqp_readint: 

        tt = D->args[D->arg_num];
        while (ptr < ptr_e && *ptr >= '0' && *ptr <= '9') {
          if (tt >= /*214748364*/ 0x7fffffffffffffffLL / 10) {
            c->parse_state = mqp_skiptoeoln;
            break;
          }
          tt = tt*10 + (*ptr - '0');
          D->query_flags &= ~4;
          ptr++;
        }
        D->args[D->arg_num] = tt;
        if (ptr == ptr_e) {
          break;
        }
        if (D->query_flags & 2) {
          D->args[D->arg_num] = -tt;
        }
        D->arg_num++;
        if ((D->query_flags & 4) || (*ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n')) {
          c->parse_state = mqp_skiptoeoln;
          break;
        }
        c->parse_state = mqp_readints;
        break;

      case mqp_skiptoeoln:
        
        D->query_flags |= 16;
        /* fallthrough */

      case mqp_readtoeoln:

        while (ptr < ptr_e && (*ptr != '\r' && *ptr != '\n')) {
          if ((unsigned char) *ptr < ' ' && *ptr != '\t') {
            D->query_flags |= 16;
          }
          ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        c->parse_state = mqp_eoln;
        /* fallthrough */
      case mqp_eoln:

        assert (ptr < ptr_e && (*ptr == '\r' || *ptr == '\n'));

        if (*ptr == '\r') {
          ptr++;
        }
        c->parse_state = mqp_wantlf;
        /* fallthrough */
      case mqp_wantlf:

        if (ptr == ptr_e) {
          break;
        }
        if (*ptr == '\n') {
          ptr++;
        }
        c->parse_state = mqp_done;

      case mqp_done:
        break;

      default:
        assert (0);
      }
    }

    len = ptr - ptr_s;
    nbit_advance (&c->Q, len);
    D->query_len += len;

    if (c->parse_state == mqp_done) {
      if (D->key_len > MAX_KEY_LEN) {
        D->query_flags |= 16;
      }
      if (!(D->query_flags & 16)) {
        c->status = conn_running;

        if ((D->crypto_flags & 10) == 2 && D->query_type == mct_delete && MCS_FUNC(c)->mc_init_crypto && D->key_len > 10) {
          static char key_buffer[MAX_KEY_LEN+4];
          nb_iterator_t it;
          nbit_set (&it, &c->In);
          assert (nbit_advance (&it, D->key_offset) == D->key_offset);
          assert (nbit_read_in (&it, key_buffer, D->key_len) == D->key_len);

          if (!strncmp (key_buffer, "@#$AuTh$#@", 10)) {
            assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);

            vkprintf(4, "mc got AUTH: delete '%s'\n", key_buffer);

            if (c->In.total_bytes) {
              c->status = conn_error;
              c->error = -1;
              return 0;
            }
            
            key_buffer[D->key_len] = 0;
            int res = MCS_FUNC(c)->mc_init_crypto (c, key_buffer + 10, D->key_len - 10);

            if (res < 0) {
              write_out (&c->Out, "NOT_FOUND\r\n", 11);

              D->crypto_flags &= ~2;

              if (!(D->crypto_flags & 1)) {
                c->status = conn_error;
                c->error = -1;
                return 0;
              }
            } else {
              D->crypto_flags |= 8;
              D->crypto_flags &= ~1;
            }

            c->status = conn_expect_query;
            c->parse_state = mqp_start;
            nbit_set (&c->Q, &c->In);
            return 0;
          }
        }

        if (!(D->crypto_flags & 9)) {
          c->status = conn_error;
          c->error = -1;
          return 0;
        }

        if (!MCS_FUNC(c)->execute) {
          MCS_FUNC(c)->execute = mcs_execute;
        }
        vkprintf(4, "mc c->pending_queries = %d, c->status = %d\n", c->pending_queries, c->status);
        int res = MCS_FUNC(c)->execute (c, D->query_type);
        vkprintf(4, "mc c->pending_queries = %d, c->status = %d, res = %d\n", c->pending_queries, c->status, res);
        if (res > 0) {
          c->status = conn_reading_query;
          return res;   // need more bytes
        } else if (res < 0) {
          if (res == SKIP_ALL_BYTES) {
            assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);
          } else {
            assert (-res <= D->query_len);
            assert (advance_skip_read_ptr (&c->In, -res) == -res);
          }
        }
        if (c->status == conn_running) {
          c->status = conn_expect_query;
        }

        assert ((c->pending_queries && (c->status == conn_wait_net || c->status == conn_wait_aio)) || (!c->pending_queries && c->status == conn_expect_query) || c->status == conn_error);

        if (c->status == conn_wait_net || c->status == conn_wait_aio) {
          c->flags |= C_REPARSE;
        }

      } else {
        if (!(D->crypto_flags & 9)) {
          c->status = conn_error;
          c->error = -1;
          return 0;
        }
        assert (advance_skip_read_ptr (&c->In, D->query_len) == D->query_len);
        c->status = conn_expect_query;
      }
      if (D->query_flags & 32) {
        assert (c->status == conn_expect_query);
        write_out (&c->Out, "SERVER_ERROR\r\n", 14);
      } else if (D->query_flags & 16) {
        assert (c->status == conn_expect_query);
        write_out (&c->Out, "CLIENT_ERROR\r\n", 14);
      }
      if (c->status != conn_wait_aio) {
        c->parse_state = mqp_start;
      }
      mcs_pad_response (c);
      nbit_set (&c->Q, &c->In);
    }
  }
  if (c->status == conn_reading_query || c->status == conn_wait_aio) {
    return NEED_MORE_BYTES;
  }
  return 0;
}

void mcs_pad_response (struct connection *c) {
  if (c->status == conn_expect_query && (MCS_DATA(c)->crypto_flags & 8)) {
    int pad_bytes = c->type->crypto_needed_output_bytes (c);
    if (pad_bytes > 0) {
      static char pad_str[16] = {'\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n'};
      assert (pad_bytes <= 15);
      write_out (&c->Out, pad_str, pad_bytes);
    }
  }
}

int return_one_key (struct connection *c, const char *key, const char *value, int value_len) {
  return return_one_key_flags_len (c, key, strlen (key), value, value_len, 0);
}

int return_one_key_flags (struct connection *c, const char *key, const char *value, int value_len, int flags) {
  return return_one_key_flags_len (c, key, strlen (key), value, value_len, flags);
}

int return_one_key_flags_len (struct connection *c, const char *key, int key_len, const char *value, int value_len, int flags) {
  write_out (&c->Out, "VALUE ", 6);
  write_out (&c->Out, key, key_len);

  char buff[27];
  int l = snprintf (buff, sizeof (buff), " %d %d\r\n", flags, value_len);
  assert (l < sizeof (buff));
  write_out (&c->Out, buff, l);

  write_out (&c->Out, value, value_len);
  write_out (&c->Out, "\r\n", 2);
  return 0;
}

int return_one_key_false (struct connection *c, const char *key, int key_len) {
  write_out (&c->Out, "VALUE ", 6);
  write_out (&c->Out, key, key_len);
  write_out (&c->Out, " 1 4\r\nb:0;\r\n", 12);
  return 0;
}

int return_one_key_list (struct connection *c, const char *key, int key_len, int res, int mode, const int *R, int R_cnt) {
  int w, i;
  const size_t buff_size = 16;
  static char buff[buff_size];

  vkprintf(4, "result = %d\n", res);

  if (!R_cnt) {
    if (res == 0x7fffffff) {
      return return_one_key (c, key, "", 0);
    }
    if (mode < 0) {
      w = 4;
      *((int *) buff) = res;
    } else {
      w = snprintf(buff, buff_size, "%d", res);
    }
    return return_one_key (c, key, buff, w);
  }

  write_out (&c->Out, "VALUE ", 6);
  write_out (&c->Out, key, key_len);

  const size_t ptr_size = 512;
  char *ptr = get_write_ptr (&c->Out, ptr_size);
  if (!ptr) return -1;
  char *s = ptr + 480;

  memcpy (ptr, " 0 .........\r\n", 14);
  char *size_ptr = ptr + 3;

  ptr += 14;
  if (res != 0x7fffffff) {
    if (mode >= 0) {
      w = snprintf(ptr, ptr_size - 14, "%d", res);
    } else {
      w = 4;
      *(reinterpret_cast<int *>(ptr)) = res;
    }
    ptr += w;
  } else {
    w = 0;
  }

  for (i = 0; i < R_cnt; i++) {
    int t;
    if (ptr >= s) {
      advance_write_ptr (&c->Out, ptr - (s - 480));
      ptr = get_write_ptr (&c->Out, 512);
      if (!ptr) {
        return -1;
      }
      s = ptr + 480;
    }
    if (mode >= 0) {
      if (i || res != 0x7fffffff) {
        *ptr++ = ',';  w++;
      }
      w += t = snprintf(ptr, 12, "%d", R[i]);
    } else {
      w += t = 4;
      *(reinterpret_cast<int *>(ptr)) = R[i];
    }
    ptr += t;
  }
  size_ptr[snprintf(size_ptr, 12, "% 9d", w)] = '\r';
  memcpy(ptr, "\r\n", 2);
  ptr += 2;
  advance_write_ptr (&c->Out, ptr - (s - 480));

  return 0;
}

int return_one_key_list_long(struct connection *c, const char *key, int key_len, int res, int mode, const long long *R, int R_cnt) {
  int w, i;
  const size_t buf_size = 16;
  static char buff[buf_size];

  vkprintf(4, "result = %d\n", res);

  if (!R_cnt) {
    if (res == 0x7fffffff) {
      return return_one_key(c, key, "", 0);
    }
    if (mode < 0) {
      w = 8;
      *(reinterpret_cast<long long *>(buff)) = res;
    } else {
      w = snprintf(buff, buf_size, "%d", res);
    }
    return return_one_key(c, key, buff, w);
  }

  write_out(&c->Out, "VALUE ", 6);
  write_out(&c->Out, key, key_len);

  char *ptr = get_write_ptr(&c->Out, 512);
  if (!ptr) {
    return -1;
  }
  char *s = ptr + 480;

  memcpy(ptr, " 0 .........\r\n", 14);
  char *size_ptr = ptr + 3;

  ptr += 14;
  if (res != 0x7fffffff) {
    if (mode >= 0) {
      w = snprintf(ptr, 12, "%d", res);
    } else {
      w = 8;
      *(reinterpret_cast<long long *>(ptr)) = res;
    }
    ptr += w;
  } else {
    w = 0;
  }

  for (i = 0; i < R_cnt; i++) {
    int t;
    if (ptr >= s) {
      advance_write_ptr(&c->Out, ptr - (s - 480));
      ptr = get_write_ptr(&c->Out, 512);
      if (!ptr) {
        return -1;
      }
      s = ptr + 480;
    }
    if (mode >= 0) {
      if (i || res != 0x7fffffff) {
        *ptr++ = ',';
        w++;
      }
      w += t = snprintf(ptr, 21, "%lld", R[i]);
    } else {
      w += t = 8;
      *(reinterpret_cast<long long *>(ptr)) = R[i];
    }
    ptr += t;
  }
  size_ptr[snprintf(size_ptr, 12, "% 9d", w)] = '\r';
  memcpy(ptr, "\r\n", 2);
  ptr += 2;
  advance_write_ptr(&c->Out, ptr - (s - 480));

  return 0;
}

int mcs_store (struct connection *c __attribute__((unused)),
               int op __attribute__((unused)),
               const char *key __attribute__((unused)),
               int key_len __attribute__((unused)),
               int flags __attribute__((unused)),
               int delay __attribute__((unused)),
               int size __attribute__((unused))) {
  return -2;
}

int mcs_get_start (struct connection *c __attribute__((unused))) {
  return 0;
}

int mcs_get (struct connection *c __attribute__((unused)), 
             const char *key __attribute__((unused)),
             int key_len __attribute__((unused))) {
  return 0;
}

int mcs_get_end (struct connection *c, int key_count __attribute__((unused))) {
  write_out (&c->Out, "END\r\n", 5);
  return 0;
}

int mcs_incr (struct connection *c, 
              int op __attribute__((unused)),
              const char *key __attribute__((unused)),
              int key_len __attribute__((unused)),
              long long arg __attribute__((unused))) {
  write_out (&c->Out, "NOT_FOUND\r\n", 11);
  return 0;
}

int mcs_delete (struct connection *c, const char *key __attribute__((unused)), int key_len __attribute__((unused))) {
  write_out (&c->Out, "NOT_FOUND\r\n", 11);
  return 0;
}

int mcs_stats (struct connection *c) {
  const char *stats = engine_default_char_stats();
  write_out (&c->Out, stats, (int)strlen(stats));
  write_out (&c->Out, "END\r\n", 5);
  return 0;
}

int mcs_version (struct connection *c) {
  write_out (&c->Out, "VERSION 2.3.9\r\n", 15);
  return 0;
}

static int mcs_wakeup (struct connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
    MCS_FUNC(c)->mc_wakeup (c);
  }
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

int mcs_alarm (struct connection *c) {
  MCS_FUNC(c)->mc_alarm (c);
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  return 0;
}

int mcs_do_wakeup (struct connection *c) {
  struct mcs_data *D = MCS_DATA(c);
  if (D->query_type == mct_get) {
    write_out (&c->Out, "END\r\n", 5);
  }
  return 0;
}


int mcs_init_accepted (struct connection *c) {
  if (MCS_FUNC(c)->mc_check_perm) {
    int res = MCS_FUNC(c)->mc_check_perm (c);
    if (res < 0) {
      return res;
    }
    if (!(res &= 3)) {
      return -1;
    }

    MCS_DATA(c)->crypto_flags = res;
  } else {
    MCS_DATA(c)->crypto_flags = 1;
  }
  
  return 0;
}



int mcs_default_check_perm (struct connection *c) {
  if (aes_initialized <= 0) {
    return (!is_same_data_center(c, 0)) ? 0 : 1;
  }
  return (!is_same_data_center(c, 0)) ? 2 : 3;
}


int mcs_init_crypto (struct connection *c, char *key, int key_len) {
//  fprintf (stderr, "mcs_init_crypto (%p [fd=%d], '%.*s')\n", c, c->fd, key_len, key);

  if (c->crypto) {
    return -1;
  }
  if (key_len != 43 || key[0] != 'A' || key[1] != ':' || key[10] != ':' || key[43]) {
    return -1;
  }

  if (c->In.total_bytes) {
    return -1;
  }

  char *tmp;

  int utime = strtoul (key + 2, &tmp, 16);
  if (tmp != key + 10) {
    return -1;
  }

  int mytime = time (nullptr);

//  fprintf (stderr, "remote time %d, local %d\n", utime, mytime);

  if (abs (mytime - utime) > 10) {
    return -1;
  }

  const size_t out_buf_size = 64;
  static char nonce_in[16], nonce_out[16], out_buf[out_buf_size];

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

  if (aes_create_connection_keys (0, default_aes_key, &aes_keys, 0, nonce_out, nonce_in, 0, utime, c) < 0) {
    return -1;
  }

  if (aes_crypto_init (c, &aes_keys, sizeof (aes_keys)) < 0) {
    return -1;
  }

  write_out(&c->Out, out_buf, snprintf(out_buf, out_buf_size, "NONCE %016llx%016llx\r\n", *(long long *)nonce_out, *(long long *)(nonce_out + 8)));

  mark_all_processed (&c->Out);
  mark_all_unprocessed (&c->In);

  return 1;
}


/*
 *
 *              END (MEMCACHED SERVER)
 *
 */
