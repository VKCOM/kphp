// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-tcp-connections.h"

#include <assert.h>
#include <cinttypes>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

#include "common/crypto/aes256.h"
#include "common/kprintf.h"
#include "common/options.h"

#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-msg-buffers.h"
#include "net/net-msg.h"

static int tcp_buffers_number = MAX_TCP_RECV_BUFFERS;
static int tcp_buffers_size = MAX_TCP_RECV_BUFFER_SIZE;
int tcp_buffers = 1;

static void parse_tcp_buffers(const char *buffers, int *number, int *size) {
  *number = MAX_TCP_RECV_BUFFERS;
  *size = MAX_TCP_RECV_BUFFER_SIZE;

  if (buffers) {
    errno = 0;
    char *buffers_number_end;
    const unsigned long buffers_number = strtoul(buffers, &buffers_number_end, 10);
    if (buffers_number_end == buffers || errno || *buffers_number_end++ != ':') {
      kprintf("Parsing tcp buffers fail: %s\n", buffers);
      usage_and_exit();
      exit(1);
    }
    assert(buffers_number <= MAX_TCP_RECV_BUFFERS);

    const long long buffers_size = parse_memory_limit(buffers_number_end);
    assert(buffers_size <= MAX_TCP_RECV_BUFFER_SIZE);

    *number = buffers_number;
    *size = buffers_size;
  }
}

OPTION_PARSER(OPT_NETWORK, "tcp-buffers", optional_argument,
              "new tcp buffers. Argument <number:size> is number and size of buffers used for reading, default 128:1k") {
  tcp_buffers = 1;
  parse_tcp_buffers(optarg, &tcp_buffers_number, &tcp_buffers_size);
  return 0;
}

int tcp_free_connection_buffers(struct connection *c) {
  rwm_free(&c->in);
  rwm_free(&c->in_u);
  rwm_free(&c->out);
  rwm_free(&c->out_p);
  return 0;
}

int tcp_prepare_iovec(struct iovec *iov, int *iovcnt, int maxcnt, raw_message_t *raw) {
  int t = rwm_prepare_iovec(raw, iov, maxcnt, raw->total_bytes);
  if (t < 0) {
    *iovcnt = maxcnt;
    int i;
    t = 0;
    for (i = 0; i < maxcnt; i++) {
      t += iov[i].iov_len;
    }
    assert(t < raw->total_bytes);
    return t;
  } else {
    *iovcnt = t;
    return raw->total_bytes;
  }
}

int tcp_recv_buffers_num;
int tcp_recv_buffers_total_size;
struct iovec tcp_recv_iovec[MAX_TCP_RECV_BUFFERS + 1];
msg_buffer_t *tcp_recv_buffers[MAX_TCP_RECV_BUFFERS];

int prealloc_tcp_buffers() {
  assert(!tcp_recv_buffers_num);

  int i;
  for (i = tcp_buffers_number - 1; i >= 0; i--) {
    msg_buffer_t *X = alloc_msg_buffer(tcp_buffers_size);
    if (!X) {
      kprintf("cannot allocate tcp receive buffer : calling exit(2)\n");
      exit(2);
    }
    tvkprintf(net_connections, 4, "allocated %d byte tcp receive buffer #%d at %p\n", msg_buffer_size(X), i, X);
    tcp_recv_buffers[i] = X;
    tcp_recv_iovec[i + 1].iov_base = X->data;
    tcp_recv_iovec[i + 1].iov_len = msg_buffer_size(X);
    ++tcp_recv_buffers_num;
    tcp_recv_buffers_total_size += msg_buffer_size(X);
  }
  return tcp_recv_buffers_num;
}

/* returns # of bytes in c->Out remaining after all write operations;
   anything is written if (1) C_WANTWR is set
                      AND (2) c->Out.total_bytes > 0 after encryption
                      AND (3) C_NOWR is not set
   if c->Out.total_bytes becomes 0, C_WANTWR is cleared ("nothing to write") and C_WANTRD is set
   if c->Out.total_bytes remains >0, C_WANTRD is cleared ("stop reading until all bytes are sent")
    15.08.2013: now C_WANTRD is never cleared anymore (we don't understand what bug we were fixing originally by this)
*/
int tcp_server_writer(struct connection *c) {
  tvkprintf(net_connections, 3, "tcp server writer on conn %d\n", c->fd);
  int r, s, t = 0, check_watermark;

  assert(c->status != conn_connecting);

  if (c->crypto) {
    assert(c->type->crypto_encrypt_output(c) >= 0);
  }

  raw_message_t *out = c->crypto ? &c->out_p : &c->out;
  do {
    check_watermark = (out->total_bytes >= c->write_low_watermark);
    while ((c->flags & C_WANTWR) != 0) {
      // write buffer loop
      s = out->total_bytes;
      if (!s) {
        c->flags &= ~C_WANTWR;
        break;
      }

      if (c->flags & C_NOWR) {
        break;
      }

      static struct iovec iov[IOV_MAX];
      int iovcnt = -1;

      s = tcp_prepare_iovec(iov, &iovcnt, IOV_MAX, out);
      assert(iovcnt > 0 && s > 0);

      r = writev(c->fd, iov, iovcnt);

      tvkprintf(net_connections, 4, "send/writev() to %d: %d written out of %d in %d chunks\n", c->fd, r, s, iovcnt);

      if (r > 0) {
        rwm_fetch_data(out, 0, r);
        t += r;
      }

      if (r < s) {
        c->flags |= C_NOWR;
      }
    }

    if (t) {
      if (check_watermark && out->total_bytes < c->write_low_watermark && c->type->ready_to_write) {
        c->type->ready_to_write(c);
        t = 0;
        if (c->crypto) {
          assert(c->type->crypto_encrypt_output(c) >= 0);
        }
        if (out->total_bytes > 0) {
          c->flags |= C_WANTWR;
        }
      }
    }
  } while ((c->flags & (C_WANTWR | C_NOWR)) == C_WANTWR);

  if (out->total_bytes) {
    // c->flags &= ~C_WANTRD;	// 15.08.2013: commented out
  } else if (c->status != conn_write_close && !(c->flags & C_FAILED)) {
    c->flags |= C_WANTRD; // 15.08.2013: unlikely to be useful without the line above
  }

  return out->total_bytes;
}

/* reads and parses as much as possible, and returns:
   0 : all ok
   <0 : have to skip |res| bytes before invoking parse_execute
   >0 : have to read that much bytes before invoking parse_execute
   -1 : if c->error has been set
   NEED_MORE_BYTES=0x7fffffff : need at least one byte more
*/
static int tcp_server_reader_inner(struct connection *c, bool once) {
  int res = 0, r, r1, s;

  raw_message_t *in = c->crypto ? &c->in_u : &c->in;
  raw_message_t *cin = &c->in;

  while (true) {
    /* check whether it makes sense to try to read from this socket */
    int try_read = (c->flags & C_WANTRD) && !(c->flags & (C_NORD | C_FAILED | C_STOPREAD)) && !c->error;
    /* check whether it makes sense to invoke parse_execute() even if no new bytes are read */
    int try_reparse =
      (c->flags & C_REPARSE)
      && (c->status == conn_expect_query || c->status == conn_reading_query || c->status == conn_wait_answer || c->status == conn_reading_answer)
      && !c->skip_bytes;
    if (!try_read && !try_reparse) {
      break;
    }

    if (try_read) {
      /* Reader */
      if (c->status == conn_write_close) {
        rwm_clear(&c->in);
        rwm_clear(&c->in_u);
        c->flags &= ~C_WANTRD;
        break;
      }

      if (!tcp_recv_buffers_num) {
        prealloc_tcp_buffers();
      }

      if (in->last && in->last->next) {
        fork_message_chain(in);
      }
      int p;
      if (c->basic_type != ct_pipe) {
        s = tcp_recv_buffers_total_size;
        if (in->last && in->last_offset != msg_buffer_size(in->last->buffer)) {
          tcp_recv_iovec[0].iov_len = msg_buffer_size(in->last->buffer) - in->last_offset;
          tcp_recv_iovec[0].iov_base = in->last->buffer->data + in->last_offset;
          p = 0;
        } else {
          p = 1;
        }

        char buffer[CMSG_SPACE(sizeof(struct ucred))];
        struct msghdr msg = {.msg_name = NULL,
                             .msg_namelen = 0,
                             .msg_iov = tcp_recv_iovec + p,
                             .msg_iovlen = static_cast<decltype(std::declval<msghdr>().msg_iovlen)>(tcp_buffers_number + 1 - p),
                             .msg_control = buffer,
                             .msg_controllen = sizeof(buffer),
                             .msg_flags = 0};

        r = recvmsg(c->fd, &msg, MSG_DONTWAIT);
        if (r >= 0) {
          assert(!(msg.msg_flags & MSG_TRUNC || msg.msg_flags & MSG_CTRUNC));
          tvkprintf(net_connections, 4, "Ancillary data size: %" PRIu64 "\n", static_cast<int64_t>(msg.msg_controllen));

          if (c->type->ancillary_data_received) {
            for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
              c->type->ancillary_data_received(c, cmsg);
            }
          }
        }
      } else {
        p = 1;
        s = tcp_recv_iovec[1].iov_len;
        r = read(c->fd, tcp_recv_iovec[1].iov_base, tcp_recv_iovec[1].iov_len);
      }

      if (r < s || once) {
        c->flags |= C_NORD;
      }
      tvkprintf(net_connections, 4, "recv() from %d: %d read out of %d. Crypto = %d\n", c->fd, r, s, c->crypto != 0);
      if (r < 0 && errno != EAGAIN) {
        tvkprintf(net_connections, 4, "recv(): %s\n", strerror(errno));
      }

      if (r > 0) {
        msg_part_t *mp = 0;
        if (!in->last) {
          assert(p == 1);
          mp = new_msg_part(tcp_recv_buffers[p - 1]);
          assert(tcp_recv_buffers[p - 1]->data == tcp_recv_iovec[p].iov_base);
          mp->offset = 0;
          mp->len = r > tcp_recv_iovec[p].iov_len ? tcp_recv_iovec[p].iov_len : r;
          r -= mp->len;
          in->first = in->last = mp;
          in->total_bytes = mp->len;
          in->first_offset = 0;
          in->last_offset = mp->len;
          p++;
        } else {
          assert(in->last->offset + in->last->len == in->last_offset);
          if (p == 0) {
            int t = r > tcp_recv_iovec[0].iov_len ? tcp_recv_iovec[0].iov_len : r;
            in->last->len += t;
            in->total_bytes += t;
            in->last_offset += t;
            r -= t;
            p++;
          }
        }

        assert(in->last && !in->last->next);

        int rs = r;
        while (rs > 0) {
          mp = new_msg_part(tcp_recv_buffers[p - 1]);
          mp->offset = 0;
          mp->len = rs > tcp_recv_iovec[p].iov_len ? tcp_recv_iovec[p].iov_len : rs;
          rs -= mp->len;
          in->last->next = mp;
          in->last = mp;
          in->last_offset = mp->len + mp->offset;
          in->total_bytes += mp->len;
          p++;
        }
        assert(!rs);

        int i;
        for (i = 0; i < p - 1; i++) {
          msg_buffer_t *X = alloc_msg_buffer(tcp_buffers_size);
          if (!X) {
            kprintf("**FATAL**: cannot allocate tcp receive buffer\n");
            exit(2);
          }
          tcp_recv_buffers[i] = X;
          tcp_recv_iovec[i + 1].iov_base = X->data;
          tcp_recv_iovec[i + 1].iov_len = msg_buffer_size(X);
        }

        s = c->skip_bytes;

        if (s && c->crypto) {
          assert(c->type->crypto_decrypt_input(c) >= 0);
        }

        r1 = c->in.total_bytes;

        if (s < 0) {
          // have to skip s more bytes
          if (r1 > -s) {
            r1 = -s;
          }

          rwm_fetch_data(cin, 0, r1);
          c->skip_bytes = s += r1;
          tvkprintf(net_connections, 4, "skipped %d bytes, %d more to skip\n", r1, -s);
          if (s) {
            continue;
          }
        }

        if (s > 0) {
          // need to read s more bytes before invoking parse_execute()
          if (r1 >= s) {
            c->skip_bytes = s = 0;
          }
          tvkprintf(net_connections, 4, "fetched %d bytes, %d available bytes, %d more to load\n", r, r1, s ? s - r1 : 0);
          if (s) {
            continue;
          }
        }
      }
    } else {
      r = 0x7fffffff;
    }

    if (c->crypto) {
      assert(c->type->crypto_decrypt_input(c) >= 0);
    }

    while (!c->skip_bytes
           && (c->status == conn_expect_query || c->status == conn_reading_query || c->status == conn_wait_answer || c->status == conn_reading_answer)) {
      /* Parser */
      int conn_expect = (c->status - 1) | 1; // one of conn_expect_query and conn_wait_answer; using VALUES of these constants!
      c->flags &= ~C_REPARSE;
      if (!cin->total_bytes) {
        /* encrypt output; why here? */
        if (c->crypto) {
          assert(c->type->crypto_encrypt_output(c) >= 0);
        }
        return 0;
      }
      if (c->status == conn_expect) {
        c->parse_state = 0;
        c->status++; // either conn_reading_query or conn_reading_answer
      }
      res = c->type->parse_execute(c);
      // 0 - ok/done, >0 - need that much bytes, <0 - skip bytes, or NEED_MORE_BYTES
      if (!res) {
        if (c->status == conn_expect + 1) { // either conn_reading_query or conn_reading_answer
          c->status--;
        }
        if (c->error) {
          return -1;
        }
      } else if (res != NEED_MORE_BYTES) {
        // have to load or skip abs(res) bytes before invoking parse_execute
        if (res < 0) {
          assert(!cin->total_bytes);
          res -= cin->total_bytes;
        } else {
          res += cin->total_bytes;
        }
        c->skip_bytes = res;
        break;
      }
    }

    if (r <= 0) {
      break;
    }
  }

  if (c->crypto) {
    /* encrypt output once again; so that we don't have to check c->Out.unprocessed_bytes afterwards */
    assert(c->type->crypto_encrypt_output(c) >= 0);
  }

  return res;
}

int tcp_server_reader(struct connection *c) {
  tvkprintf(net_connections, 3, "tcp server reader on conn %d\n", c->fd);
  return tcp_server_reader_inner(c, true);
}

int tcp_server_reader_till_end(struct connection *c) {
  tvkprintf(net_connections, 3, "tcp server reader till end on conn %d\n", c->fd);
  return tcp_server_reader_inner(c, false);
}



/* 0 = all ok, >0 = so much more bytes needed to encrypt last block */
int tcp_aes_crypto_encrypt_output(struct connection *c) {
  struct aes_crypto *T = static_cast<aes_crypto*>(c->crypto);
  assert(c->crypto);
  raw_message_t *out = &c->out;

  int l = out->total_bytes;
  l &= ~15;
  if (l) {
    /*    assert (rwm_encrypt_decrypt_cbc (out, l, &T->write_aeskey, T->write_iv) == l);
        if (out->total_bytes & 15) {
          raw_message_t x;
          rwm_split_head (&x, out, l);
          rwm_union (&c->out_p, &x);
        } else {
          rwm_union (&c->out_p, &c->out);
          rwm_init (&c->out, 0);
        }*/
    rwm_encrypt_decrypt_to_callback_t cb = reinterpret_cast<rwm_encrypt_decrypt_to_callback_t>(T->write_aeskey.cbc_crypt);
    assert(rwm_encrypt_decrypt_to(&c->out, &c->out_p, l, &T->write_aeskey, cb, T->write_iv) == l);
  }

  return (-out->total_bytes) & 15;
}

/* 0 = all ok, >0 = so much more bytes needed to decrypt last block */
int tcp_aes_crypto_decrypt_input(struct connection *c) {
  struct aes_crypto *T = static_cast<aes_crypto*>(c->crypto);
  assert(c->crypto);
  raw_message_t *in = &c->in_u;

  int l = in->total_bytes;
  l &= ~15;
  if (l) {
    rwm_encrypt_decrypt_to_callback_t cb = reinterpret_cast<rwm_encrypt_decrypt_to_callback_t>(T->read_aeskey.cbc_crypt);
    assert(rwm_encrypt_decrypt_to(&c->in_u, &c->in, l, &T->read_aeskey, cb, T->read_iv) == l);
  }

  return (-in->total_bytes) & 15;
}

/* returns # of bytes needed to complete last output block */
int tcp_aes_crypto_needed_output_bytes(struct connection *c) {
  assert(c->crypto);
  return -c->out.total_bytes & 15;
}
