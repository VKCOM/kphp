// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-memcache-client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/kprintf.h"
#include "common/precise-time.h"

#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-events.h"
#include "net/net-sockaddr-storage.h"

/*
 *
 *		MEMCACHED CLIENT INTERFACE
 *
 */


#define	MAX_KEY_LEN	1000
#define	MAX_VALUE_LEN	1048576

static int mcc_parse_execute (struct connection *c);
static int mcc_connected (struct connection *c);

enum mc_query_parse_state {
  mrp_start,
  mrp_readcommand,
  mrp_skipspc,
  mrp_readkey,
  mrp_readint,
  mrp_readints,
  mrp_skiptoeoln,
  mrp_readtoeoln,
  mrp_eoln,
  mrp_wantlf,
  mrp_done
};

conn_type_t ct_memcache_client = {
  .magic = CONN_FUNC_MAGIC,
  .flags = 0,
  .title = "memcache_client",
  .accept = server_failed,
  .init_accepted = server_failed,
  .create_outbound = NULL,
  .run = server_read_write,
  .reader = server_reader,
  .writer = server_writer,
  .close = client_close_connection,
  .free_buffers = free_connection_buffers,
  .parse_execute = mcc_parse_execute,
  .init_outbound = mcc_init_outbound,
  .connected = mcc_connected,
  .wakeup = server_noop,
  .alarm = NULL,
  .ready_to_write = NULL,
  .check_ready = mc_client_check_ready,
  .wakeup_aio = NULL,
  .data_received = NULL,
  .data_sent = NULL,
  .ancillary_data_received = NULL,
  .flush = NULL,
  .crypto_init = aes_crypto_init,
  .crypto_free = aes_crypto_free,
  .crypto_encrypt_output = aes_crypto_encrypt_output,
  .crypto_decrypt_input = aes_crypto_decrypt_input,
  .crypto_needed_output_bytes = aes_crypto_needed_output_bytes
};

int mcc_execute (struct connection *c, int op);

struct memcache_client_functions default_memcache_client = {
  .info = NULL,
  .execute = mcc_execute,
  .check_ready = server_check_ready,
  .flush_query = mcc_flush_query,
  .connected = NULL,
  .mc_check_perm = mcc_default_check_perm,
  .mc_init_crypto = mcc_init_crypto,
  .mc_start_crypto = mcc_start_crypto
};


int mcc_execute (struct connection *c __attribute__((unused)), int op __attribute__((unused))) {
  return -1;
}

static int mcc_parse_execute (struct connection *c) {
  struct mcc_data *D = MCC_DATA(c);
  char *ptr, *ptr_s, *ptr_e;
  int len;
  long long tt;

  while (true) {
    len = nbit_ready_bytes (&c->Q);
    ptr = ptr_s = static_cast<char*>(nbit_get_ptr (&c->Q));
    ptr_e = ptr + len;
    if (len <= 0) {
      break;
    }

    while (ptr < ptr_e && c->parse_state != mrp_done) {

      switch (c->parse_state) {
      case mrp_start:
        D->clen = 0;
        D->response_flags = 0;
        D->response_type = mcrt_none;
        D->response_len = 0;
	D->key_offset = 0;
	D->key_len = 0;
	D->arg_num = 0;
        c->parse_state = mrp_readcommand;
        /* fallthrough */
      case mrp_readcommand:
        if (!D->clen && *ptr == '~') {
          ptr++;
          D->response_flags = 1;
        }
        while (ptr < ptr_e && D->clen < 15 && ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || (*ptr == '_'))) {
          D->comm[D->clen++] = *ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        
        if (D->clen == 0 && ((*ptr >= '0' && *ptr <= '9') || *ptr == '-')) {
          D->response_type = mcrt_NUMBER;
          c->parse_state = mrp_readints;
	  D->args[0] = 0;
          break;
        }

        if (D->clen == 15 || (*ptr != '\t' && *ptr != ' ' && *ptr != '\r' && *ptr != '\n')) {
          c->parse_state = mrp_skiptoeoln;
          break;
        }
        D->comm[D->clen] = 0;
        switch (D->comm[0]) {
        case 'V':
          if (!strcmp (D->comm, "VALUE")) {
            D->response_type = mcrt_VALUE;
          } else if (!strcmp (D->comm, "VERSION")) {
            D->response_type = mcrt_VERSION;
          }
          break;
        case 'S':
          if (!strcmp (D->comm, "STORED")) {
            D->response_type = mcrt_STORED;
          } else
          if (!strcmp (D->comm, "SERVER_ERROR")) {
            D->response_type = mcrt_SERVER_ERROR;
          }
          break;
        case 'N':
          if (!strcmp (D->comm, "NOT_STORED")) {
            D->response_type = mcrt_NOTSTORED;
          } else 
          if (!strcmp (D->comm, "NOT_FOUND")) {
            D->response_type = mcrt_NOTFOUND;
          }
          if (!strcmp (D->comm, "NONCE")) {
            D->response_type = mcrt_NONCE;
          }
          break;
        case 'C':
          if (!strcmp (D->comm, "CLIENT_ERROR")) {
            D->response_type = mcrt_CLIENT_ERROR;
          }
          break;
        case 'E':
          if (!strcmp (D->comm, "END")) {
            D->response_type = mcrt_END;
          } else
          if (!strcmp (D->comm, "ERROR")) {
            D->response_type = mcrt_ERROR;
          }
          break;
        case 'D':
          if (!strcmp (D->comm, "DELETED")) {
            D->response_type = mcrt_DELETED;
          }
          break;
        case 0:
          D->response_type = mcrt_empty;
          break;
        default:
          break;
        }

        if (D->response_type == mcrt_none) {
          c->parse_state = mrp_skiptoeoln;
          break;
        }
        

	D->key_offset = D->clen;
	c->parse_state = mrp_skipspc;
      /* fallthrough */

      case mrp_skipspc:

	while (ptr < ptr_e && (*ptr == ' ' || *ptr == '\t')) {
	  D->key_offset++;
          ptr++;
	}
	if (ptr == ptr_e) {
          break;
	}
	if (*ptr != '\r' && *ptr != '\n') {
	  if (D->response_type >= mcrt_STORED) {
	    c->parse_state = mrp_skiptoeoln;
	    break;
	  } else if (D->response_type >= mcrt_VERSION) {
	    c->parse_state = mrp_readtoeoln;
	    break;
	  } else {
	    c->parse_state = mrp_readkey;
	  }
        } else {
          c->parse_state = mrp_eoln;
          break;
        }
       
      case mrp_readkey:

	while (ptr < ptr_e && (unsigned char) *ptr > ' ') {
	  D->key_len++;
          ptr++;
	}
	if (ptr == ptr_e) {
          break;
	}
	if (*ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n') {
	  c->parse_state = mrp_skiptoeoln;
	  break;
	}
	D->arg_num = 0;

	if (D->response_type == mcrt_NONCE) {
	  c->parse_state = (*ptr == '\r' || *ptr == '\n') ? mrp_eoln : mrp_skiptoeoln;
	  break;
	}

	c->parse_state = mrp_readints;
  /* fallthrough */

      case mrp_readints:

	while (ptr < ptr_e && (*ptr == ' ' || *ptr == '\t')) {
          ptr++;
	}
	if (ptr == ptr_e) {
          break;
	}
	if (*ptr == '\r' || *ptr == '\n') {
	  c->parse_state = mrp_eoln;
	  break;
	}
	if (D->arg_num >= 4) {
	  c->parse_state = mrp_skiptoeoln;
	  break;
	}
	D->args[D->arg_num] = 0;
	if (*ptr == '-') {
	  ptr++;
	  D->response_flags |= 6;
	} else {
	  D->response_flags |= 4;
	}
	c->parse_state = mrp_readint;
        /* fallthrough */
      case mrp_readint:	

        tt = D->args[D->arg_num];
        while (ptr < ptr_e && *ptr >= '0' && *ptr <= '9') {
          if (tt > 0x7fffffffffffffffLL / 10 || (tt == 0x7fffffffffffffffLL / 10 && *ptr > '0' + (char) (0x7fffffffffffffffLL % 10) + ((D->response_flags & 2) != 0))) {
            kprintf ("number too large in memcache answer - already %lld\n", tt);
            c->parse_state = mrp_skiptoeoln;
            goto mrp_skiptoeoln;
          }
          tt = tt*10 + (*ptr - '0');
          D->response_flags &= ~4;
          ptr++;
        }
        D->args[D->arg_num] = tt;
        if (ptr == ptr_e) {
          break;
        }
        if (D->response_flags & 2) {
          D->args[D->arg_num] = -tt;
        }
        D->arg_num++;
        if ((D->response_flags & 4) || (*ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n')) {
	  c->parse_state = mrp_skiptoeoln;
	  break;
        }
        c->parse_state = mrp_readints;
        break;

      case mrp_skiptoeoln:  mrp_skiptoeoln:

        kprintf ("mrp_skiptoeoln, response_type=%d, remainder=%.*s\n", D->response_type, ptr_e - ptr < 64 ? (int)(ptr_e - ptr) : 64, ptr);

        D->response_flags |= 16;
        /* fallthrough */

      case mrp_readtoeoln:

        while (ptr < ptr_e && (*ptr != '\r' && *ptr != '\n')) {
	  if ((unsigned char) *ptr < ' ' && *ptr != '\t' && !(D->response_flags & 16)) {
	    c->parse_state = mrp_skiptoeoln;
	    goto mrp_skiptoeoln;
	  }
          ptr++;
        }
        if (ptr == ptr_e) {
          break;
        }
        c->parse_state = mrp_eoln;
        /* fallthrough */
      case mrp_eoln:

        assert (ptr < ptr_e && (*ptr == '\r' || *ptr == '\n'));

        if (*ptr == '\r') {
          ptr++;
        }
        c->parse_state = mrp_wantlf;
        /* fallthrough */
      case mrp_wantlf:

        if (ptr == ptr_e) {
          break;
        }
        if (*ptr == '\n') {
          ptr++;
        }
        c->parse_state = mrp_done;

      case mrp_done:
        break;

      default:
        assert (0);
      }
    }

    len = ptr - ptr_s;
    nbit_advance (&c->Q, len);
    D->response_len += len;

    if (D->response_flags & 48) {
      kprintf("bad response from memcache server at %s, connection %d\n", sockaddr_storage_to_string(&c->remote_endpoint), c->fd);
      c->error = -1;
      return 0;
    }

    if (c->parse_state == mrp_done) {
      if (D->key_len > MAX_KEY_LEN) {
        kprintf ("error: key len %d exceeds %d, response_type=%d\n", D->key_len, MAX_KEY_LEN, D->response_type);
        D->response_flags |= 16;
      }
      if (!(D->response_flags & 16)) {
        c->status = conn_running;

        if (!MCC_FUNC(c)->execute) {
          MCC_FUNC(c)->execute = mcc_execute;
        }

        int res = 0;

        if ((D->crypto_flags & 14) == 6) {
          switch (D->response_type) {
          case mcrt_NONCE:
            if (MCC_FUNC(c)->mc_start_crypto && D->key_len > 0 && D->key_len <= 255) {
              static char nonce_key[256];
              assert (advance_skip_read_ptr (&c->In, D->key_offset) == D->key_offset);
              assert (read_in (&c->In, nonce_key, D->key_len) == D->key_len);
              nonce_key[D->key_len] = 0;
              int t = D->response_len - D->key_offset - D->key_len;
              assert (advance_skip_read_ptr (&c->In, t) == t);
              if (t == 2) {
                res = MCC_FUNC(c)->mc_start_crypto (c, nonce_key, D->key_len);
                if (res > 0) {
                  D->crypto_flags = (D->crypto_flags & -16) | 10;
                  MCC_FUNC(c)->flush_query (c);
                  res = 0;
                  break;
                }
              }
            }
            c->status = conn_error;
            c->error = -1;
            return 0;
          case mcrt_NOTFOUND:
            if (D->crypto_flags & 1) {
              assert (advance_skip_read_ptr (&c->In, D->response_len) == D->response_len);
              release_all_unprocessed (&c->Out);
              D->crypto_flags = 1;
              break;
            }
            /* fallthrough */
          default:
            c->status = conn_error;
            c->error = -1;
            return 0;
          }
        } else if (D->response_type == mcrt_NONCE) {
	          kprintf("bad response in NONCE from memcache server at %s, connection %d\n", sockaddr_storage_to_string(&c->remote_endpoint), c->fd);
            c->status = conn_error;
            c->error = -1;
            return 0;
        } else {

          res = MCC_FUNC(c)->execute (c, D->response_type);

        }

        if (res > 0) {
          c->status = conn_reading_answer;
          return res;	// need more bytes
        } else if (res < 0) {
          if (res == SKIP_ALL_BYTES) {
            assert (advance_skip_read_ptr (&c->In, D->response_len) == D->response_len);
          } else {
            assert (-res <= D->response_len);
            assert (advance_skip_read_ptr (&c->In, -res) == -res);
          }
        }
        if (c->status == conn_running) {
          c->status = conn_wait_answer;
        }
      } else {
        assert (advance_skip_read_ptr (&c->In, D->response_len) == D->response_len);
        c->status = conn_wait_answer;
      }
      if (D->response_flags & 48) {
        //write_out (&c->Out, "CLIENT_ERROR\r\n", 14);
	      kprintf("bad response from memcache server at %s, connection %d\n", sockaddr_storage_to_string(&c->remote_endpoint), c->fd);
        c->status = conn_error;
        c->error = -1;
	return 0;
      }
      if (c->status == conn_error && c->error < 0) {
        return 0;
      }
      c->parse_state = mrp_start;
      nbit_set (&c->Q, &c->In);
    }
  }
  if (c->status == conn_reading_answer) {
    return NEED_MORE_BYTES;
  }
  return 0;
}

int mcc_connected (struct connection *c) {
  c->last_query_sent_time = precise_now;

  vkprintf(4, "connection #%d: connected, crypto_flags = %d\n", c->fd, MCC_DATA(c)->crypto_flags);
  if (MCC_FUNC(c)->mc_check_perm) {
    int res = MCC_FUNC(c)->mc_check_perm (c);
    if (res < 0) {
      return res;
    }
    if (!(res &= 3)) {
      return -1;
    }
    MCC_DATA(c)->crypto_flags = res;
  } else {
    MCC_DATA(c)->crypto_flags = 3;
  }

  if ((MCC_DATA(c)->crypto_flags & 2) && MCC_FUNC(c)->mc_init_crypto) {
    int res = MCC_FUNC(c)->mc_init_crypto (c);

    if (res > 0) {
      assert (MCC_DATA(c)->crypto_flags & 4);
    } else if (!(MCC_DATA(c)->crypto_flags & 1)) {
      return -1;
    }
  }

  write_out (&c->Out, "version\r\n", 9);

  //arseny30: added for php-engine:
  if (MCC_FUNC(c)->connected) {
    MCC_FUNC(c)->connected (c);
  }

  assert (MCC_FUNC(c)->flush_query);
  MCC_FUNC(c)->flush_query (c);

  return 0;
}


int mc_client_check_ready (struct connection *c) {
  return MCC_FUNC(c)->check_ready (c);
}



int mcc_init_outbound (struct connection *c) {
  c->last_query_sent_time = precise_now;

  if (MCC_FUNC(c)->mc_check_perm) {
    int res = MCC_FUNC(c)->mc_check_perm (c);
    if (res < 0) {
      return res;
    }
    if (!(res &= 3)) {
      return -1;
    }

    MCC_DATA(c)->crypto_flags = res;
  } else {
    MCC_DATA(c)->crypto_flags = 1;
  }

  return 0;
}

int mcc_default_check_perm (struct connection *c) {
  if (aes_initialized <= 0) {
    return (!is_same_data_center(c, 1)) ? 0 : 1;
  }
  return (!is_same_data_center(c, 1)) ? 2 : 1;
}

/*int mcc_default_check_perm (struct connection *c) {
  if ((c->remote_ip & 0xff000000) != 0x0a000000 && (c->remote_ip & 0xff000000) != 0x7f000000) {
    return 0;
  }
  if (!c->our_ip) {
    return 3;
  }
  int ipxor = (c->our_ip ^ c->remote_ip);
  int mask = (c->our_ip & 0x00800000) ? 0xfff00000 : 0xffe00000;

  if (aes_initialized <= 0) {
    return (ipxor & mask) ? 0 : 1;
  }
  return (ipxor & mask) ? 2 : 1;
  }*/


int mcc_init_crypto (struct connection *c) {
  MCC_DATA(c)->nonce_time = time (0);

  aes_generate_nonce (MCC_DATA(c)->nonce);

  const size_t buf_size = 128;
  static char buf[buf_size];

  write_out(&c->Out, buf, snprintf(buf, buf_size, "delete @#$AuTh$#@A:%08x:%016llx%016llx\r\n",
    MCC_DATA(c)->nonce_time,
    *(long long *)MCC_DATA(c)->nonce,
    *(long long *)(MCC_DATA(c)->nonce + 8))
  );

  assert ((MCC_DATA(c)->crypto_flags & 14) == 2);
  MCC_DATA(c)->crypto_flags |= 4;
 
  mark_all_processed (&c->Out);

  return 1;
}

int mcc_start_crypto (struct connection *c, char *key, int key_len) {
  struct mcc_data *D = MCC_DATA(c);

  if (c->crypto) {
    return -1;
  }
  if (key_len != 32 || key[32]) {
    return -1;
  }

  if (c->In.total_bytes || c->Out.total_bytes || !D->nonce_time) {
    return -1;
  }

  static char nonce_in[16];
  char *tmp;

  *(long long *)(nonce_in + 8) = strtoull (key + 16, &tmp, 16);
  if (tmp != key + 32) {
    return -1;
  }

  key[16] = 0;
  *(long long *)nonce_in = strtoull (key, &tmp, 16);
  if (tmp != key + 16) {
    return -1;
  }

  struct aes_session_key aes_keys;

  if (aes_create_connection_keys (0, default_aes_key, &aes_keys, 1, nonce_in, D->nonce, 0, D->nonce_time, c) < 0) {
    return -1;
  }

  if (aes_crypto_init (c, &aes_keys, sizeof (aes_keys)) < 0) {
    return -1;
  }

  mark_all_unprocessed (&c->In);

  return 1;
}

int mcc_flush_query (struct connection *c) {
  if (c->crypto) {
    int pad_bytes = c->type->crypto_needed_output_bytes (c);
    vkprintf(4, "mc: flush query (padding with %d bytes)\n", pad_bytes);
    if (pad_bytes > 0) {
      static char pad_str[16] = {'\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n'};
      assert (pad_bytes <= 15);
      write_out (&c->Out, pad_str, pad_bytes);
    }
  }
  return flush_connection_output (c);
}


/*
 *
 *		END (MEMCACHED CLIENT)
 *
 */
