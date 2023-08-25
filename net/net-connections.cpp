// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-connections.h"

#include <arpa/inet.h>
#include <assert.h>
#include <cinttypes>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "common/container_of.h"
#include "common/crc32.h"
#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/pid.h"
#include "common/precise-time.h"
#include "common/server/limits.h"
#include "common/server/relogin.h"
#include "common/stats/provider.h"
#include "common/tl/constants/common.h"
#include "common/tl/constants/net.h"
#include "common/tl/parse.h"

#include "net/net-buffers.h"
#include "net/net-crypto-aes.h"
#include "net/net-events.h"
#include "net/net-ifnet.h"
#include "net/net-msg-buffers.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-socket-options.h"
#include "net/net-socket.h"

#define USE_EPOLLET 0
#define MAX_RECONNECT_INTERVAL 20

DEFINE_VERBOSITY(net_connections)

static int bucket_salt;

int max_connection;
struct connection *Connections;

static int fd_fake_connection = 0;
static int fd_fake_connection_end = MAX_CONNECTIONS;

int active_connections, active_special_connections, max_special_connections = MAX_CONNECTIONS;
int outbound_connections, active_outbound_connections, ready_outbound_connections, listening_connections;
long long outbound_connections_created, inbound_connections_accepted;
int ready_targets;
int conn_generation;
double last_conn_start_processing = 0;
static void(*on_active_special_connections_update_callback)() = []{};

const char *unix_socket_directory = "/var/run/engine";
SAVE_STRING_OPTION_PARSER(OPT_NETWORK, "unix-socket-directory", unix_socket_directory, "path to directory with UNIX sockets");

long long netw_queries, netw_update_queries, total_failed_connections, total_connect_failures, unused_connections_closed;

static void connections_constructor() __attribute__((constructor));
static void connections_constructor() {
  // Second half is used for fake connections
  Connections = static_cast<connection*>(calloc(2 * MAX_CONNECTIONS, sizeof(Connections[0])));
  assert(Connections && "Cannot allocate memory for Connections");
  bucket_salt = lrand48();
}

void set_on_active_special_connections_update_callback(void (*callback)()) noexcept {
  assert(callback);
  on_active_special_connections_update_callback = callback;
}

int free_tmp_buffers(struct connection *c) {
  if (c->Tmp) {
    free_all_buffers(c->Tmp);
    c->Tmp = 0;
  }
  return 0;
}

/* default value for conn->type->free_buffers */
int free_connection_buffers(struct connection *c) {
  free_tmp_buffers(c);
  free_all_buffers(&c->In);
  free_all_buffers(&c->Out);
  return 0;
}

int server_writer(struct connection *c);
int client_init_outbound(struct connection *c __attribute__((unused))) { return 0; }

/*
 * client
 */

double get_recent_idle_percent() {
  const double a_idle_time = epoll_average_idle_time();
  const double a_idle_quotient = epoll_average_idle_quotient();
  return a_idle_quotient > 0 ? a_idle_time / a_idle_quotient * 100 : a_idle_time;
}

STATS_PROVIDER(net, 1000) {
  int uptime = get_uptime();

  stats->add_general_stat("tot_idle_time", "%.3f", epoll_total_idle_time());
  stats->add_general_stat("average_idle_percent", "%.3f", uptime > 0 ? epoll_total_idle_time() / uptime * 100 : 0);
  stats->add_histogram_stat("recent_idle_percent", get_recent_idle_percent());

  stats->add_general_stat("network_connections", "%d", active_connections);
  stats->add_general_stat("encrypted_connections", "%d", allocated_aes_crypto);
  stats->add_general_stat("max_network_connections", "%d", maxconn);
  stats->add_general_stat("inbound_connections_accepted", "%lld", inbound_connections_accepted);
  stats->add_general_stat("outbound_connections_created", "%lld", outbound_connections_created);
  stats->add_general_stat("outbound_connections", "%d", outbound_connections);
  stats->add_general_stat("active_outbound_connections", "%d", active_outbound_connections);
  stats->add_general_stat("ready_outbound_connections", "%d", ready_outbound_connections);
  stats->add_general_stat("ready_targets", "%d", ready_targets);
  stats->add_general_stat("allocated_targets", "%d", allocated_targets);
  stats->add_general_stat("declared_targets", "%d", active_targets);
  stats->add_general_stat("inactive_targets", "%d", inactive_targets);
  stats->add_general_stat("active_special_connections", "%d", active_special_connections);
  stats->add_general_stat("max_special_connections", "%d", max_special_connections);
  stats->add_general_stat("active_network_events", "%d", epoll_event_heap_size());
  stats->add_general_stat("active_timers", "%d", epoll_timer_heap_size());
  stats->add_general_stat("used_network_buffers", "%d", NB_used);
  stats->add_general_stat("free_network_buffers", "%d", NB_free);
  stats->add_general_stat("allocated_network_buffers", "%d", NB_alloc);
  stats->add_general_stat("max_network_buffers", "%d", NB_max);
  stats->add_general_stat("network_buffer_size", "%d", NB_size);
  stats->add_histogram_stat("queries_total", netw_queries);
  stats->add_general_stat("qps", "%.3f", safe_div(netw_queries, uptime));
  stats->add_general_stat("update_queries_total", "%lld", netw_update_queries);
  stats->add_general_stat("update_qps", "%.3f", safe_div(netw_update_queries, uptime));

  stats->add_general_stat("PID", "%s", pid_to_print(&PID));
}

int prepare_iovec(struct iovec *iov, int *iovcnt, int maxcnt, netbuffer_t *H, int max_bytes) {
  int t = 0, i;
  nb_iterator_t Iter;
  nbit_set(&Iter, H);

  for (i = 0; i < maxcnt && max_bytes; i++) {
    int s = nbit_ready_bytes(&Iter);
    if (s <= 0) {
      break;
    }
    if (s > max_bytes) {
      s = max_bytes;
    }
    iov[i].iov_len = s;
    iov[i].iov_base = nbit_get_ptr(&Iter);
    assert(nbit_advance(&Iter, s) == s);
    t += s;
    max_bytes -= s;
  }

  *iovcnt = i;

  return t;
}

int set_write_timer(struct connection *c);

/* returns # of bytes in c->Out remaining after all write operations;
   anything is written if (1) C_WANTWR is set
                      AND (2) c->Out.total_bytes > 0 after encryption
                      AND (3) C_NOWR is not set
   if c->Out.total_bytes becomes 0, C_WANTWR is cleared ("nothing to write") and C_WANTRD is set
   if c->Out.total_bytes remains >0, C_WANTRD is cleared ("stop reading until all bytes are sent")
    15.08.2013: now C_WANTRD is never cleared anymore (we don't understand what bug we were fixing originally by this)
*/
int server_writer(struct connection *c) {
  tvkprintf(net_connections, 3, "server write to conn %d\n", c->fd);
  int r, s, t = 0, check_watermark;
  char *to;

  assert(c->status != conn_connecting);

  if (c->crypto) {
    assert(c->type->crypto_encrypt_output(c) >= 0);
  }

  do {
    check_watermark = (c->Out.total_bytes >= c->write_low_watermark);
    while ((c->flags & C_WANTWR) != 0) {
      // write buffer loop
      s = get_ready_bytes(&c->Out);

      if (!s) {
        c->flags &= ~C_WANTWR;
        break;
      }

      if (c->flags & C_NOWR) {
        break;
      }

      static struct iovec iov[64];
      int iovcnt = -1;

      int max_bytes = 1 << 30;

      if (c->limit_per_sec) {
        if (c->last_write_time != now) {
          c->written_per_sec = 0;
        }
        max_bytes = c->limit_per_sec - c->written_per_sec;
        if (max_bytes <= 0) {
          set_write_timer(c);
          break;
        }
        tvkprintf(net_connections, 4, "limited write to connection %d by %d bytes\n", c->fd, max_bytes);
      }

      if (c->limit_per_write && max_bytes > c->limit_per_write) {
        max_bytes = c->limit_per_write;
      }

      s = prepare_iovec(iov, &iovcnt, 64, &c->Out, max_bytes);
      assert(iovcnt > 0 && s > 0);

      to = get_read_ptr(&c->Out);

      r = writev(c->fd, iov, iovcnt);

      if (r < 0) {
        if (errno == EAGAIN) {
          if (++c->eagain_count > 100) {
            kprintf("Too much EAGAINs for connection %d (%s), dropping\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint));
            fail_connection(c, -123);
          }
        } else {
          tvkprintf(net_connections, 1, "writev(): %m\n");
        }
      } else {
        c->eagain_count = 0;
      }

      tvkprintf(net_connections, 4, "send/writev() to %d: %d written out of %d in %d chunks at %p (%.*s)\n",
                c->fd, r, s, iovcnt, to, ((unsigned)r < 64) ? r : 64, to);


      if (r > 0) {
        advance_skip_read_ptr(&c->Out, r);
        t += r;
        if (c->limit_per_sec) {
          c->written_per_sec += r;
          c->last_write_time = now;
        }
        if (c->type->data_sent) {
          c->type->data_sent(c, r);
        }
      }

      if (r < s) {
        c->flags |= C_NOWR;
      }

      if ((c->flags & C_FAILED) || c->error) {
        c->flags |= C_NOWR;
        break;
      }
    }

    if (t) {
      free_unused_buffers(&c->Out);
      if (check_watermark && c->Out.total_bytes < c->write_low_watermark) {
        if (c->type->ready_to_write) {
          c->type->ready_to_write(c);
        }
        t = 0;
        if (c->crypto) {
          assert(c->type->crypto_encrypt_output(c) >= 0);
        }
        if (c->Out.total_bytes > 0) {
          c->flags |= C_WANTWR;
        }
      }
    }
  } while ((c->flags & (C_WANTWR | C_NOWR)) == C_WANTWR);

  if (c->Out.total_bytes) {
  } else if (c->status != conn_write_close && !(c->flags & C_FAILED)) {
    c->flags |= C_WANTRD; // 15.08.2013: unlikely to be useful without the line above
  }

  return c->Out.total_bytes;
}

/* reads and parses as much as possible, and returns:
   0 : all ok
   <0 : have to skip |res| bytes before invoking parse_execute
   >0 : have to read that much bytes before invoking parse_execute
   -1 : if c->error has been set
   NEED_MORE_BYTES=0x7fffffff : need at least one byte more
*/
int server_reader(struct connection *c) {
  tvkprintf(net_connections, 3, "server read from conn %d\n", c->fd);
  int res = 0, r, r1, s;
  char *to;

  while (true) {
    /* check whether it makes sense to try to read from this socket */
    int try_read = (c->flags & C_WANTRD) && !(c->flags & (C_NORD | C_FAILED | C_STOPREAD)) && !c->error;
    /* check whether it makes sense to invoke parse_execute() even if no new bytes are read */
    int try_reparse = (c->flags & C_REPARSE) && (c->status == conn_expect_query || c->status == conn_reading_query || c->status == conn_wait_answer ||
                                                 c->status == conn_reading_answer) &&
                      !c->skip_bytes;
    if (!try_read && !try_reparse) {
      break;
    }

    if (try_read) {
      /* Reader */
      if (c->status == conn_write_close) {
        free_all_buffers(&c->In);
        c->flags &= ~C_WANTRD;
        break;
      }

      to = get_write_ptr(&c->In, 512);
      s = get_write_space(&c->In);

      if (s <= 0) {
        kprintf("error while reading from connection #%d (type %p(%s); in.bytes=%d, out.bytes=%d, tmp.bytes=%d; peer %s): cannot allocate read buffer\n",
                 c->fd, c->type, c->type ? c->type->title : "", c->In.total_bytes + c->In.unprocessed_bytes, c->Out.total_bytes + c->Out.unprocessed_bytes,
                 c->Tmp ? c->Tmp->total_bytes : 0, sockaddr_storage_to_string(&c->remote_endpoint));

        cond_dump_connection_buffers_stats();
        free_all_buffers(&c->In);
        c->error = -1;
        return -1;
      }

      assert(to && s > 0);

      if (c->basic_type != ct_pipe) {
        struct iovec iovec = {.iov_base = to, .iov_len = static_cast<size_t>(s)};
        char buffer[CMSG_SPACE(sizeof(struct ucred))];
        struct msghdr msg =
          {.msg_name = NULL, .msg_namelen = 0, .msg_iov = &iovec, .msg_iovlen = 1, .msg_control = buffer, .msg_controllen = sizeof(buffer), .msg_flags = 0};

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
        r = read(c->fd, to, s);
      }

      if (r < s) {
        c->flags |= C_NORD;
      }

      tvkprintf(net_connections, 4, "recv() from %d: %d read out of %d\n", c->fd, r, s);
      if (r < 0 && errno != EAGAIN) {
        tvkprintf(net_connections, 1, "recv(): %s\n", strerror(errno));
      }

      if (r > 0) {

        advance_write_ptr(&c->In, r);

        s = c->skip_bytes;

        if (s && c->crypto) {
          assert(c->type->crypto_decrypt_input(c) >= 0);
        }

        if (c->type->data_received) {
          c->type->data_received(c, r);
        }

        if ((c->flags & C_FAILED) || c->error) {
          return -1;
        }

        r1 = c->In.total_bytes;

        if (s < 0) {
          // have to skip s more bytes
          if (r1 > -s) {
            r1 = -s;
          }
          advance_read_ptr(&c->In, r1);
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

    while (!c->skip_bytes &&
           (c->status == conn_expect_query || c->status == conn_reading_query || c->status == conn_wait_answer || c->status == conn_reading_answer)) {
      /* Parser */
      int conn_expect = (c->status - 1) | 1; // one of conn_expect_query and conn_wait_answer; using VALUES of these constants!
      c->flags &= ~C_REPARSE;
      if (!c->In.total_bytes) {
        /* encrypt output; why here? */
        if (c->crypto) {
          assert(c->type->crypto_encrypt_output(c) >= 0);
        }
        return 0;
      }
      if (c->status == conn_expect) {
        nbit_set(&c->Q, &c->In);
        c->parse_state = 0;
        c->status++; // either conn_reading_query or conn_reading_answer
      } else if (!nbit_ready_bytes(&c->Q)) {
        break;
      }
      res = c->type->parse_execute(c);
      // 0 - ok/done, >0 - need that much bytes, <0 - skip bytes, or NEED_MORE_BYTES
      if (!res) {
        nbit_clear(&c->Q);
        if (c->status == conn_expect + 1) { // either conn_reading_query or conn_reading_answer
          c->status--;
        }
        if (c->error) {
          return -1;
        }
      } else if (res != NEED_MORE_BYTES) {
        // have to load or skip abs(res) bytes before invoking parse_execute
        if (res < 0) {
          assert(!c->In.total_bytes);
          res -= c->In.total_bytes;
        } else {
          res += c->In.total_bytes;
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

int clear_connection_write_timeout(struct connection *c);

int server_close_connection(struct connection *c, int who __attribute__((unused))) {
  tvkprintf(net_connections, 3, "server close conn %d\n", c->fd);
  struct conn_query *q;

  clear_connection_timeout(c);
  clear_connection_write_timeout(c);

  if (c->first_query) {
    while (c->first_query != (struct conn_query *)c) {
      q = c->first_query;
      q->cq_type->close(q);
      if (c->first_query == q) {
        delete_conn_query(q);
      }
    }
  }

  if (c->type->crypto_free) {
    c->type->crypto_free(c);
  }

  if (c->target || c->next) {
    c->next->prev = c->prev;
    c->prev->next = c->next;
    c->prev = c->next = 0;
  }

  if (c->target) {
    --c->target->outbound_connections;
    --outbound_connections;
    if (c->status != conn_connecting) {
      --c->target->active_outbound_connections;
      --active_outbound_connections;
    }
    clean_unused_target(c->target);
  }

  c->status = conn_none;
  c->flags = 0;
  c->generation = -1;

  if (c->basic_type == ct_listen) {
    return 0;
  }

  return c->type->free_buffers(c);
}

void compute_next_reconnect(conn_target_t *S) {
  if (S->next_reconnect_timeout < S->reconnect_timeout || S->active_outbound_connections) {
    S->next_reconnect_timeout = S->reconnect_timeout;
  }
  S->next_reconnect = precise_now + S->next_reconnect_timeout;
  if (!S->active_outbound_connections && S->next_reconnect_timeout < MAX_RECONNECT_INTERVAL) {
    S->next_reconnect_timeout = S->next_reconnect_timeout * 1.5 + drand48() * 0.2;
  }
}

int client_close_connection(struct connection *c, int who __attribute__((unused))) {
  tvkprintf(net_connections, 3, "client close conn %d\n", c->fd);
  struct conn_query *q;
  conn_target_t *S = c->target;

  clear_connection_timeout(c);
  clear_connection_write_timeout(c);

  if (c->first_query) {
    while (c->first_query != (struct conn_query *)c) {
      q = c->first_query;
      q->cq_type->close(q);
      if (c->first_query == q) {
        delete_conn_query(q);
      }
    }
  }

  if (c->type->crypto_free) {
    c->type->crypto_free(c);
  }

  if (S) {
    c->next->prev = c->prev;
    c->prev->next = c->next;
    --S->outbound_connections;
    --outbound_connections;
    if (c->status != conn_connecting) {
      --S->active_outbound_connections;
      --active_outbound_connections;
    }
    if (S->outbound_connections < S->min_connections && precise_now >= S->next_reconnect && S->refcnt > 0) {
      create_new_connections(S);
      if (S->next_reconnect <= precise_now) {
        compute_next_reconnect(S);
      }
    }
    clean_unused_target(S);
  }

  c->status = conn_none;
  c->flags = 0;
  c->generation = -1;

  return c->type->free_buffers(c);
}

#if USE_EPOLLET
static inline int compute_conn_events(struct connection *c) {
  int events = EVT_SPEC;

  if (c->flags & C_WANTRD) {
    if (!(c->flags & C_STOPREAD)) {
      events |= EVT_READ;
    }
  }

  if (c->flags & C_WANTWR) {
    events |= EVT_WRITE;
  }

  return events;
}
#else
static inline int compute_conn_events(struct connection *c) {
  int events = EVT_SPEC;

  if (c->flags & C_WANTRD) {
    if (!(c->flags & C_STOPREAD)) {
      events |= EVT_READ;
    }
    if (c->flags & C_NORD) {
      events |= EVT_LEVEL;
    }
  }

  if (c->flags & C_WANTWR) {
    events |= EVT_WRITE;
    if (c->flags & C_NOWR) {
      events |= EVT_LEVEL;
    }
  }

  return events;
}
#endif

void close_special_connection(struct connection *c) {
  tvkprintf(net_connections, 3, "close special conn %d\n", c->fd);
  if (c->basic_type != ct_listen) {
    --active_special_connections;
    on_active_special_connections_update_callback();
    if (active_special_connections < max_special_connections && Connections[c->listening].basic_type == ct_listen &&
        Connections[c->listening].generation == c->listening_generation) {
      epoll_insert(c->listening, EVT_READ | EVT_LEVEL);
    }
  }
}

int force_clear_connection(struct connection *c) {
  tvkprintf(net_connections, 3, "force clean conn %d\n", c->fd);
  if (c->status != conn_connecting) {
    active_connections--;
    if (c->flags & C_SPECIAL) {
      close_special_connection(c);
    }
  } else {
    total_connect_failures++;
  }
  c->type->close(c, 0);
  clear_connection_timeout(c);
  clear_connection_write_timeout(c);

  if (c->ev) {
    c->ev->data = 0;
  }
  memset(c, 0, sizeof(struct connection));

  return 1;
}

void net_reset_after_fork() {
  //Epoll_close should clear internal structures but shouldn't change epoll_fd.
  //The same epoll_fd will be used by master
  //Solution: close epoll_fd first
  //Problems: "epoll_ctl(): Invalid argument" is printed to stderr
  close_epoll();
  init_epoll();

  for (int i = 0; i < allocated_targets; i++) {
    while (Targets[i].refcnt > 0) {
      destroy_target(&Targets[i]);
    }
  }

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    connection_t *conn = &Connections[i];
    if (conn->status == conn_none) {
      continue;
    }
    close(i);
    clear_event(i);
    force_clear_connection(conn);
  }

  active_outbound_connections = 0;
  active_connections = 0;

  reset_PID();
}

int out_total_processed_bytes(struct connection *c) {
  if (c->flags & C_RAWMSG) {
    return (c->crypto ? c->out_p.total_bytes : c->out.total_bytes);
  } else {
    return c->Out.total_bytes;
  }
}

int out_total_unprocessed_bytes(struct connection *c) {
  if (c->flags & C_RAWMSG) {
    return (c->crypto ? c->out.total_bytes : 0);
  } else {
    return c->Out.unprocessed_bytes;
  }
}

int server_read_write(struct connection *c) {
  int res, inconn_mask = c->flags | ~C_INCONN;
  event_t *ev = c->ev;

  tvkprintf(net_connections, 3, "begin processing connection %d, status=%d, flags=%d, pending=%d; epoll_ready=%d, ev->ready=%d\n", c->fd, c->status, c->flags, c->pending_queries,
           ev->epoll_ready, ev->ready);

  c->flags |= C_INCONN;

  if (!c->interrupted && ev->epoll_ready & (EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLPRI)) {
    if ((ev->epoll_ready & EPOLLIN) && (c->flags & C_WANTRD) && !(c->flags & (C_NORD | C_FAILED | C_STOPREAD))) {
      tvkprintf(net_connections, 4, "reading buffered data from socket %d, before closing\n", c->fd);
      c->type->reader(c);
    }
    tvkprintf(net_connections, 3 + !(ev->epoll_ready & EPOLLPRI), "socket %d: disconnected (epoll_ready=%02x), closing ignored %d\n", c->fd, ev->epoll_ready, c->ignored);
    // When connection is closed on client side, we have two options to do:
    // (1) Close connection on server side immediately
    // (2) Delay closing connection on server side, until we leave the ignore_user_abort "critical section"
    if (c->ignored) {
      // This handles case (2)
      // The problem is when connection is closed on client side, epoll always reports EPOLLRDHUP on the underlying fd. It leads to endless repeated wakeups.
      // To avoid this we do the trick: readd fd to epoll in edge-triggered mode.
      // In edge-triggered mode event is reported only at the first time, so we will have only one wakeup on EPOLLRDHUP after readding fd to epoll.
      c->interrupted = true; // Remember that the connection was closed if script was inside the ignore_user_abort "critical section"
      c->flags &= ~C_WANTRW; // Remove flags for re-adding fd in edge-triggered mode by using compute_conn_events() in the next calls
      return EVT_SPEC; // Use EVT_SPEC only without EVT_LEVEL to add fd in edge-triggered mode
    } else {
      // This handles case (1)
      force_clear_connection(c);
      return EVA_DESTROY;
    }
  }

  /*
    if (c->gather && (c->gather->timeout_time >= now || c->gather->ready_num == c->gather->wait_num)) {
    }
  */

  if (c->status == conn_connecting) { /* connecting... */
    if (ev->ready & EVT_WRITE) {
      tvkprintf(net_connections, 4, "socket #%d to %s becomes active\n", c->fd, sockaddr_storage_to_string(&c->target->endpoint));
      conn_target_t *S = c->target;
      S->active_outbound_connections++;
      active_outbound_connections++;
      active_connections++;
      c->status = conn_wait_answer;
      c->flags = (c->flags & C_PERMANENT) | C_WANTRD | C_INCONN;
      c->type->connected(c);

      if (out_total_processed_bytes(c) > 0) {
        c->flags |= C_WANTWR;
      }
      c->type->check_ready(c);

      tvkprintf(net_connections, 4, "socket #%d: ready=%d\n", c->fd, c->ready);
    }
    if (c->status == conn_connecting) {
      c->flags &= inconn_mask;
      kprintf("socket #%d: unknown event before connecting (?)\n", c->fd);
      return EVT_SPEC;
    }
  }

  if (c->flags & C_ALARM) {
    c->flags &= ~C_ALARM;
    c->type->alarm(c);
  } else if (c->status == conn_wait_net && !c->pending_queries) {
    c->type->wakeup(c);
  } else if (c->status == conn_wait_aio && !c->pending_queries) {
    c->type->wakeup(c);
  }

  if (c->flags & C_DFLUSH) {
    c->flags &= ~C_DFLUSH;
    c->type->flush(c);
    if (c->crypto && out_total_unprocessed_bytes(c) > 0) {
      assert(c->type->crypto_encrypt_output(c) >= 0);
    }
    if (out_total_processed_bytes(c) > 0) {
      c->flags |= C_WANTWR;
    }
  }

  /* NOTE that the following loop may run forever if WANTWR is set and there are no available bytes in output buffer; consider clearing WANTWR in else branch to
   * type->writer call */
  /* MAIN LOOP: run while either
     1) we want and can read;
     2) we want and can write;
     3) we want re-parse input */
  while (((c->flags & C_WANTRD) && !(c->flags & (C_NORD | C_FAILED | C_STOPREAD))) || ((c->flags & C_WANTWR) && !(c->flags & (C_NOWR | C_FAILED))) ||
         ((c->flags & C_REPARSE) &&
          (c->status == conn_expect_query || c->status == conn_reading_query || c->status == conn_wait_answer || c->status == conn_reading_answer))) {

    if (c->error) {
      break;
    }

    if (out_total_processed_bytes(c) + out_total_unprocessed_bytes(c) > 0) {
      res = c->type->writer(c); // if res > 0, then writer has cleared C_WANTRD and set C_NOWR
    }

    res = c->type->reader(c);
    /* res = 0 if ok;
       res != 0 on error, or if need to receive more bytes before proceeding (almost equal to c->skip_bytes);
         for conn_wait_net or conn_wait_aio (i.e. if further inbound processing is stalled),
         we get res=NEED_MORE_BYTES.
       All available bytes have been already read into c->In.
       If we have run out of buffers for c->In, c->error = -1, res = -1.
       As much output bytes have been encrypted as possible.
    */
    tvkprintf(net_connections, 4, "server_reader=%d, ready=%02x, state=%02x\n", res, c->ev->ready, c->ev->state);
    if (res || c->skip_bytes) {
      /* we have processed as much inbound queries as possible, leaving main loop */
      break;
    }

    if (out_total_processed_bytes(c) > 0) {
      c->flags |= C_WANTWR;
      res = c->type->writer(c);
    }
  }

  if (c->flags & C_DFLUSH) {
    c->flags &= ~C_DFLUSH;
    c->type->flush(c);
    if (c->crypto && out_total_unprocessed_bytes(c) > 0) {
      assert(c->type->crypto_encrypt_output(c) >= 0);
    }
  }

  if (out_total_processed_bytes(c) > 0) {
    c->flags |= C_WANTWR;
    c->type->writer(c);
    if (!(c->flags & C_RAWMSG)) {
      free_unused_buffers(&c->In);
      free_unused_buffers(&c->Out);
    }
  }

  if (c->error || c->status == conn_error || (c->status == conn_write_close && !(c->flags & C_WANTWR)) || (c->flags & C_FAILED)) {
    tvkprintf(net_connections, 1, "conn %d: closing and cleaning (error code=%d)\n", c->fd, c->error);

    if (c->interrupted) {
      // Here is delayed connection closing described above at case (2)
      force_clear_connection(c);
    } else {
      if (c->status != conn_connecting) {
        active_connections--;
        if (c->flags & C_SPECIAL) {
          close_special_connection(c);
        }
      }
      c->type->close(c, 0);
      clear_connection_timeout(c);

      memset(c, 0, sizeof(struct connection));
      ev->data = 0;
    }
    return EVA_DESTROY;
  }

  c->flags &= inconn_mask;

  tvkprintf(net_connections, 3, "finish processing connection %d, status=%d, flags=%d, pending=%d\n", c->fd, c->status, c->flags, c->pending_queries);

  return compute_conn_events(c);
}

int server_read_write_gateway(int fd __attribute__((unused)), void *data, event_t *ev) {
  tvkprintf(net_connections, 3, "server read write gateway on fd %d\n", fd);
  struct connection *c = (struct connection *)data;
  assert(c);
  assert(c->type);

  if (ev->ready & EVT_FROM_EPOLL) {
    // update C_NORD / C_NOWR only if we arrived from epoll()
    ev->ready &= ~EVT_FROM_EPOLL;
    c->flags &= ~C_NORW;
    if ((ev->state & EVT_READ) && !(ev->ready & EVT_READ)) {
      c->flags |= C_NORD;
    }
    if ((ev->state & EVT_WRITE) && !(ev->ready & EVT_WRITE)) {
      c->flags |= C_NOWR;
    }
    if (ev->epoll_ready & EPOLLERR) {
      int error;
      if (!socket_error(c->fd, &error)) {
        tvkprintf(net_connections, 1, "got error for tcp socket #%d, %s : %s\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint), strerror(error));
      }
    }
  }

  return c->type->run(c);
}

int conn_timer_wakeup_gateway(event_timer_t *et) {
  struct connection *c = container_of(et, struct connection, timer);
  tvkprintf(net_connections, 3, "ALARM: awakening connection %d at %p, status=%d, pending=%d\n", c->fd, c, c->status, c->pending_queries);
  c->flags |= C_ALARM;
  put_event_into_heap(c->ev);
  return 0;
}

int conn_write_timer_wakeup_gateway(event_timer_t *et) {
  struct connection *c = container_of(et, struct connection, write_timer);
  tvkprintf(net_connections, 3, "writer wakeup: awakening connection %d at %p, status=%d\n", c->fd, c, c->status);
  if (out_total_processed_bytes(c) + out_total_unprocessed_bytes(c) > 0) {
    c->flags = (c->flags | C_WANTWR) & ~C_NOWR;
  }
  put_event_into_heap(c->ev);
  return 0;
}

int set_connection_timeout(struct connection *c, double timeout) {
  c->timer.wakeup = conn_timer_wakeup_gateway;
  c->flags &= ~C_ALARM;
  set_timer_operation(&c->timer, "connection");
  if (timeout > 0) {
    c->timer.wakeup_time = precise_now + timeout;
    return insert_event_timer(&c->timer);
  } else {
    c->timer.wakeup_time = 0;
    return remove_event_timer(&c->timer);
  }
}

int set_write_timer(struct connection *c) {
  c->flags &= ~C_WANTWR;
  set_timer_params(&c->write_timer, conn_write_timer_wakeup_gateway, precise_now + 0.3 * (1 + drand48()), "write");
  return insert_event_timer(&c->write_timer);
}

int clear_connection_timeout(struct connection *c) {
  c->flags &= ~C_ALARM;
  c->timer.wakeup_time = 0;
  return remove_event_timer(&c->timer);
}

int clear_connection_write_timeout(struct connection *c) {
  c->write_timer.wakeup_time = 0;
  return remove_event_timer(&c->write_timer);
}

int fail_connection(struct connection *c, int err) {
  tvkprintf(net_connections, 3, "fail on conn %d, err %d\n", c->fd, err);
  if (!(c->flags & C_FAILED)) {
    if (err != -17) {
      total_failed_connections++;
      if (c->status == conn_connecting) {
        total_connect_failures++;
      }
    } else {
      unused_connections_closed++;
    }
  }
  c->flags |= C_FAILED;
  c->flags &= ~(C_WANTRD | C_WANTWR);
  if (c->status == conn_connecting) {
    c->target->active_outbound_connections++;
    active_outbound_connections++;
    active_connections++;
  }
  c->status = conn_error;
  if (c->error >= 0) {
    c->error = err;
  }
  put_event_into_heap(c->ev);
  return 0;
}

int flush_connection_output(struct connection *c) {
  if (out_total_processed_bytes(c) + out_total_unprocessed_bytes(c) > 0) {
    c->flags |= C_WANTWR;
    int res = c->type->writer(c);
    if (out_total_processed_bytes(c) > 0 && !(c->flags & C_INCONN)) {
      epoll_insert(c->fd, compute_conn_events(c));
    }
    return res;
  } else {
    return 0;
  }
}

int flush_later(struct connection *c) {
  if (out_total_processed_bytes(c) + out_total_unprocessed_bytes(c) > 0) {
    if (c->flags & C_DFLUSH) {
      return 1;
    }
    c->flags |= C_DFLUSH;
    if (!(c->flags & C_INCONN)) {
      put_event_into_heap(c->ev);
    }
    return 2;
  }
  return 0;
}

void init_connection_buffers(struct connection *c) {
  if (c->flags & C_RAWMSG) {
    c->In.state = c->Out.state = 0;
    rwm_init(&c->in, 0);
    rwm_init(&c->out, 0);
    rwm_init(&c->in_u, 0);
    rwm_init(&c->out_p, 0);
  } else {
    c->in.magic = c->out.magic = 0;
    init_builtin_buffer(&c->In, c->in_buff, BUFF_SIZE);
    init_builtin_buffer(&c->Out, c->out_buff, BUFF_SIZE);
  }
}

void clean_connection_buffers(struct connection *c) {
  if (c->flags & C_RAWMSG) {
    rwm_free(&c->in);
    rwm_free(&c->out);
    rwm_free(&c->in_u);
    rwm_free(&c->out_p);
  }
}

int accept_new_connections(struct connection *cc) {
  struct sockaddr_storage peer, self;
  socklen_t peer_addrlen, self_addrlen;
  int acc = 0;
  struct connection *c;
  event_t *ev;
  ucred creds;
  bool creds_found = false;

  assert(cc->basic_type == ct_listen);
  do {
    peer_addrlen = sizeof(peer);
    self_addrlen = sizeof(self);
    memset(&peer, 0, sizeof(peer));
    memset(&self, 0, sizeof(self));

    const int cfd = accept4(cc->fd, (struct sockaddr *)&peer, &peer_addrlen, SOCK_CLOEXEC);
    if (cfd < 0) {
      if (!acc) {
        tvkprintf(net_connections, 1, "accept(%d) unexpectedly returns %d: %m\n", cc->fd, cfd);
      }
      break;
    }
    acc++;
    inbound_connections_accepted++;
    getsockname(cfd, (struct sockaddr *)&self, &self_addrlen);
    ev = epoll_fd_event(cfd);

    if (cc->flags & C_IPV6) {
      assert(peer_addrlen == sizeof(struct sockaddr_in6));
      assert(peer.ss_family == AF_INET6);
      assert(self.ss_family == AF_INET6);
    } else if (cc->flags & C_UNIX) {
      assert(peer.ss_family == AF_UNIX);
      assert(self.ss_family == AF_UNIX);

      socklen_t len = sizeof(creds);
      if (getsockopt(cfd, SOL_SOCKET, SO_PEERCRED, &creds, &len) == 0) {
        creds_found = true;
      } else {
        kprintf("failed to dectect credentials by getsockopt, let's try message: %s\n", strerror(errno));
        if (!socket_enable_unix_passcred(cfd)) {
          kprintf("Cannot enable SO_PASSCRED on UNIX socket: %s\n", strerror(errno));
          close(cfd);
          continue;
        }
      }
      tvkprintf(net_connections, 4, "Enabled SO_PASSCRED on UNIX socket: fd %d\n", cfd);
    } else {
      assert(peer_addrlen == sizeof(struct sockaddr_in));
      assert(peer.ss_family == AF_INET);
      assert(self.ss_family == AF_INET);
    }

    char buffer_peer[SOCKADDR_STORAGE_BUFFER_SIZE], buffer_self[SOCKADDR_STORAGE_BUFFER_SIZE];
    tvkprintf(net_connections, 3, "accepted incoming connection of type %s, flags=%x, at %s -> %s, fd=%d\n", cc->type->title, cc->flags, sockaddr_storage_to_buffer(&peer, buffer_peer),
             sockaddr_storage_to_buffer(&self, buffer_self), cfd);

    int flags;
    if ((flags = fcntl(cfd, F_GETFL, 0) < 0) || fcntl(cfd, F_SETFL, flags | O_NONBLOCK) < 0) {
      kprintf("cannot set O_NONBLOCK on accepted socket %d: %m\n", cfd);
      close(cfd);
      continue;
    }
    if (cfd >= MAX_CONNECTIONS || (cfd >= maxconn && maxconn)) {
      close(cfd);
      continue;
    }
    if (cfd > max_connection) {
      max_connection = cfd;
    }
    if (!(cc->flags & C_UNIX)) {
      socket_enable_tcp_nodelay(cfd);
    }
    socket_maximize_sndbuf(cfd, 0);
    socket_maximize_rcvbuf(cfd, 0);

    c = Connections + cfd;
    memset(c, 0, sizeof(struct connection));
    c->fd = cfd;
    c->ev = ev;
    c->generation = ++conn_generation;
    c->flags = C_WANTRD;
    c->flags |= (cc->flags & C_RAWMSG) | (cc->type->flags & C_RAWMSG);

    init_connection_buffers(c);

    c->timer.wakeup = conn_timer_wakeup_gateway;
    c->write_timer.wakeup = conn_write_timer_wakeup_gateway;
    c->type = cc->type;
    c->extra = cc->extra;
    c->basic_type = ct_inbound;
    c->status = conn_expect_query;
    c->local_endpoint = self;
    c->remote_endpoint = peer;
    if (creds_found) {
      c->credentials = creds;
    }

    if (peer.ss_family == AF_INET6) {
      c->flags |= C_IPV6;
    }

    if (cc->flags & C_UNIX) {
      c->flags |= C_UNIX;
    }
    tvkprintf(net_connections, 3, "accepted connection flags: %08x\n", c->flags);

    c->first_query = c->last_query = (struct conn_query *)c;
    if (c->type->init_accepted(c) >= 0) {
      epoll_sethandler(cfd, 0, server_read_write_gateway, c);
      epoll_insert(cfd, (c->flags & C_WANTRD ? EVT_READ : 0) | (c->flags & C_WANTWR ? EVT_WRITE : 0) | EVT_SPEC);
      active_connections++;
      c->listening = cc->fd;
      c->listening_generation = cc->generation;
      if ((cc->flags & C_NOQACK)) {
        c->flags |= C_NOQACK;
      }
      c->window_clamp = cc->window_clamp;
      if (c->window_clamp) {
        if (!socket_set_tcp_window_clamp(cfd, c->window_clamp)) {
          kprintf("error while setting window size for socket %d to %d: %m\n", cfd, c->window_clamp);
        }
      }
      if (cc->flags & C_SPECIAL) {
        c->flags |= C_SPECIAL;
        if (active_special_connections >= max_special_connections) {
          tvkprintf(net_connections, active_special_connections >= max_special_connections + 16 ? 0 : 3,
                   "ERROR: forced to accept connection when special connections limit was reached (%d of %d)\n", active_special_connections,
                   max_special_connections);
        }
        ++active_special_connections;
        last_conn_start_processing = get_utime_monotonic();
        on_active_special_connections_update_callback();
        if (active_special_connections >= max_special_connections) {
          return EVA_REMOVE;
        }
      }
    } else {
      clean_connection_buffers(c);
      c->basic_type = ct_none;
      close(cfd);
    }
  } while (true);
  return EVA_CONTINUE;
}

int accept_new_connections_gateway(int fd __attribute__((unused)), void *data, event_t *ev __attribute__((unused))) {
  struct connection *cc = static_cast<connection*>(data);
  assert(cc->basic_type == ct_listen);
  assert(cc->type);
  return cc->type->accept(cc);
}

int server_check_ready(struct connection *c) {
  if (c->status == conn_none || c->status == conn_connecting) {
    return c->ready = cr_notyet;
  }
  if (c->status == conn_error || c->ready == cr_failed) {
    return c->ready = cr_failed;
  }
  return c->ready = cr_ok;
}

void ancillary_data_received(struct connection *c, const struct cmsghdr *cmsg) {
  tvkprintf(net_connections, 4, "Ancillary data received\n");

  if (c->flags & C_UNIX) {
    const void *payload = CMSG_DATA(cmsg);
    const size_t payload_size = cmsg->cmsg_len - CMSG_LEN(0);

    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS) {
      assert(payload_size == sizeof(struct ucred));
      const struct ucred *credentials = static_cast<const ucred*>(payload);
      tvkprintf(net_connections, 4, "Credentials received: PID: %d, UID: %d, GID: %d\n", credentials->pid, credentials->uid, credentials->gid);
      c->credentials = *credentials;
      assert(socket_disable_unix_passcred(c->fd));
      return;
    }
  }

  kprintf("Unexpected ancillary data received: cmsg_level %d, cmsg_type %d\n", cmsg->cmsg_level, cmsg->cmsg_type);
}

int server_noop(struct connection *c __attribute__((unused))) { return 0; }

int server_noop(struct connection *c __attribute__((unused)), int who __attribute__((unused))) { return 0; }

int server_failed(struct connection *c) {
  kprintf("connection %d: call to pure virtual method\n", c->fd);
  assert(0);
  return -1;
}

int server_failed(struct connection *c, int who __attribute__((unused))) {
  kprintf("connection %d: call to pure virtual method\n", c->fd);
  assert(0);
  return -1;
}

int check_conn_functions(conn_type_t *type) {
  if (type->magic != CONN_FUNC_MAGIC) {
    return -1;
  }
  if (!type->title) {
    type->title = "(unknown)";
  }
  if (!type->run) {
    type->run = server_read_write;
  }
  if (!type->accept) {
    type->accept = accept_new_connections;
  }
  if (!type->init_accepted) {
    type->init_accepted = server_noop;
  }
  if (!type->free_buffers) {
    type->free_buffers = free_connection_buffers;
  }
  if (!type->close) {
    type->close = server_close_connection;
  }
  if (!type->reader) {
    type->reader = server_reader;
    if (!type->parse_execute) {
      return -1;
    }
  }
  if (!type->writer) {
    type->writer = server_writer;
  }
  if (!type->init_outbound) {
    type->init_outbound = server_noop;
  }
  if (!type->wakeup) {
    type->wakeup = server_noop;
  }
  if (!type->alarm) {
    type->alarm = server_noop;
  }
  if (!type->connected) {
    type->connected = server_noop;
  }
  if (!type->flush) {
    type->flush = server_noop;
  }
  if (!type->check_ready) {
    type->check_ready = server_check_ready;
  }
  return 0;
}

int init_listening_connection_ext(int fd, conn_type_t *type, void *extra, int mode, int prio) {
  if (check_conn_functions(type) < 0) {
    return -1;
  }
  if (fd >= MAX_CONNECTIONS) {
    return -1;
  }
  if (fd > max_connection) {
    max_connection = fd;
  }
  struct connection *c = Connections + fd;

  memset(c, 0, sizeof(struct connection));

  c->fd = fd;
  c->type = type;
  c->extra = extra;
  c->basic_type = ct_listen;
  c->status = conn_listen;

  if ((mode & SM_LOWPRIO)) {
    prio = 10;
  }

  epoll_sethandler(fd, prio, accept_new_connections_gateway, c);

  epoll_insert(fd, EVT_READ | EVT_LEVEL);

  if ((mode & SM_SPECIAL)) {
    c->flags |= C_SPECIAL;
  }

  if ((mode & SM_NOQACK)) {
    c->flags |= C_NOQACK;
    assert(socket_disable_tcp_quickack(c->fd));
  }

  if ((mode & SM_IPV6)) {
    c->flags |= C_IPV6;
  }

  if ((mode & SM_UNIX)) {
    c->flags |= C_UNIX;
  }

  if ((mode & SM_RAWMSG)) {
    c->flags |= C_RAWMSG;
  }

  listening_connections++;

  return 0;
}

int init_listening_connection_mode(int fd, conn_type_t *type, void *extra, int mode) {
  return init_listening_connection_ext(fd, type, extra, mode, -10);
}

connection_t *lock_fake_connection() {
  assert(!fd_fake_connection || fd_fake_connection >= MAX_CONNECTIONS);

  if (!fd_fake_connection) {
    if (fd_fake_connection_end < 2 * MAX_CONNECTIONS) {
      fd_fake_connection = fd_fake_connection_end++;
    } else {
      return NULL;
    }
  }

  connection_t *c = &Connections[fd_fake_connection];
  const int next_fd_fake_connection = c->fd;
  c->fd = c - Connections;
  fd_fake_connection = next_fd_fake_connection;

  return c;
}

void unlock_fake_connection(connection_t *c) {
  assert(&Connections[MAX_CONNECTIONS] <= c && c < &Connections[2 * MAX_CONNECTIONS]);

  c->fd = fd_fake_connection;
  fd_fake_connection = c - Connections;
}

int default_parse_execute(struct connection *c __attribute__((unused))) { return 0; }

// Connection target handling functions

static sockaddr_storage zero_sockaddr_storage;
#define DEFINE_CONN_TARGETS(name)                                                                                                                              \
  conn_target_t name = {.min_connections = 0,                                                                                                                  \
                        .max_connections = 0,                                                                                                                  \
                        .first_conn = NULL,                                                                                                                    \
                        .last_conn = NULL,                                                                                                                     \
                        .first_query = NULL,                                                                                                                   \
                        .last_query = NULL,                                                                                                                    \
                        .type = NULL,                                                                                                                          \
                        .extra = NULL,                                                                                                                         \
                        .generation = 0,                                                                                                                       \
                        .endpoint = zero_sockaddr_storage,                                                                                                     \
                        .active_outbound_connections = 0,                                                                                                      \
                        .outbound_connections = 0,                                                                                                             \
                        .ready_outbound_connections = 0,                                                                                                       \
                        .next_reconnect = 0,                                                                                                                   \
                        .reconnect_timeout = 0,                                                                                                                \
                        .next_reconnect_timeout = 0,                                                                                                           \
                        .custom_field = 0,                                                                                                                     \
                        .refcnt = 0,                                                                                                                           \
                        .next_target = &name,                                                                                                                  \
                        .prev_target = &name,                                                                                                                  \
                        .hnext = NULL,                                                                                                                         \
                        .extra_ptr1 = NULL,                                                                                                                    \
                        .extra_ptr2 = NULL}

DEFINE_CONN_TARGETS(ActiveTargets);
static DEFINE_CONN_TARGETS(InactiveTargets);
static DEFINE_CONN_TARGETS(FreeTargets);
conn_target_t Targets[MAX_TARGETS];
conn_target_t *HTarget[PRIME_TARGETS];
int allocated_targets, active_targets, inactive_targets, free_targets;

enum lookup_type { lookup, insert, erase };
typedef enum lookup_type lookup_type_t;

static conn_target_t *find_target_inet(const struct sockaddr_in *target, conn_type_t *type, void *extra, lookup_type_t lookup_type, conn_target_t *new_target) {
  unsigned h1 = ((unsigned long)type * 0xabacaba + target->sin_addr.s_addr) % PRIME_TARGETS;
  h1 = (h1 * 239 + bucket_salt) % PRIME_TARGETS;
  conn_target_t **prev = HTarget + h1, *cur;
  while ((cur = *prev) != 0) {
    if (cur->endpoint.ss_family == AF_INET && cur->type == type && cur->extra == extra) {
      struct sockaddr_in *inet_sockaddr = cast_sockaddr_storage_to_inet(&cur->endpoint);
      if (inet_sockaddr->sin_addr.s_addr == target->sin_addr.s_addr && inet_sockaddr->sin_port == target->sin_port) {
        if (lookup_type == erase) {
          *prev = cur->hnext;
          cur->hnext = 0;
          return cur;
        }
        assert(lookup_type == lookup);
        return cur;
      }
    }
    prev = &cur->hnext;
  }
  assert(lookup_type == lookup || lookup_type == insert);
  if (lookup_type == insert) {
    new_target->hnext = HTarget[h1];
    HTarget[h1] = new_target;
    return new_target;
  }
  return 0;
}

static conn_target_t *find_target_inet6(const struct sockaddr_in6 *target, conn_type_t *type, void *extra, lookup_type_t lookup_type, conn_target_t *new_target) {
  unsigned h1 = ((unsigned long)type * 0xabacaba) % PRIME_TARGETS;
  int i;
  for (i = 0; i < 4; i++) {
    h1 = ((unsigned long long)h1 * 17239 + ((unsigned *)target->sin6_addr.s6_addr)[i]) % PRIME_TARGETS;
  }
  h1 = (h1 * 239 + bucket_salt) % PRIME_TARGETS;
  conn_target_t **prev = HTarget + h1, *cur;
  while ((cur = *prev) != 0) {
    if (cur->endpoint.ss_family == AF_INET6 && cur->type == type && cur->extra == extra) {
      struct sockaddr_in6 *inet6_sockaddr = cast_sockaddr_storage_to_inet6(&cur->endpoint);
      if (!memcmp(inet6_sockaddr->sin6_addr.s6_addr, target->sin6_addr.s6_addr, 16) && inet6_sockaddr->sin6_port == target->sin6_port) {
        if (lookup_type == erase) {
          *prev = cur->hnext;
          cur->hnext = 0;
          return cur;
        }
        assert(lookup_type == lookup);
        return cur;
      }
    }
    prev = &cur->hnext;
  }
  assert(lookup_type == lookup || lookup_type == insert);
  if (lookup_type == insert) {
    new_target->hnext = HTarget[h1];
    HTarget[h1] = new_target;
    return new_target;
  }
  return 0;
}

static conn_target_t *find_target_unix(const struct sockaddr_un *target, conn_type_t *type, void *extra, lookup_type_t lookup_type, conn_target_t *new_target) {
  const size_t length = strlen(target->sun_path);
  const unsigned bucket = compute_crc32c(target->sun_path, length) % PRIME_TARGETS;

  conn_target_t **prev = &HTarget[bucket];
  conn_target_t *cur;

  while ((cur = *prev) != 0) {
    if (cur->endpoint.ss_family == AF_UNIX && cur->type == type && cur->extra == extra) {
      struct sockaddr_un *unix_sockaddr = cast_sockaddr_storage_to_unix(&cur->endpoint);
      if (!strcmp(unix_sockaddr->sun_path, target->sun_path)) {
        if (lookup_type == erase) {
          *prev = cur->hnext;
          cur->hnext = 0;
          return cur;
        }
        assert(lookup_type == lookup);
        return cur;
      }
    }
    prev = &cur->hnext;
  }

  assert(lookup_type == lookup || lookup_type == insert);
  if (lookup_type == insert) {
    new_target->hnext = HTarget[bucket];
    HTarget[bucket] = new_target;
    return new_target;
  }

  return NULL;
}

static inline conn_target_t *find_target(const conn_target_t *source, lookup_type_t lookup_type, conn_target_t *new_target) {
  switch (source->endpoint.ss_family) {
    case AF_INET:
      return find_target_inet(cast_sockaddr_storage_to_inet(&source->endpoint), source->type, source->extra, lookup_type, new_target);
    case AF_INET6:
      return find_target_inet6(cast_sockaddr_storage_to_inet6(&source->endpoint), source->type, source->extra, lookup_type, new_target);
    case AF_UNIX:
      return find_target_unix(cast_sockaddr_storage_to_unix(&source->endpoint), source->type, source->extra, lookup_type, new_target);
    default:
      assert(0);
  }

  return NULL;
}

static inline void remove_target_from_list(conn_target_t *S) {
  S->prev_target->next_target = S->next_target;
  S->next_target->prev_target = S->prev_target;
}

static inline void insert_target_into_list(conn_target_t *S, conn_target_t *Head) {
  S->prev_target = Head->prev_target;
  S->next_target = Head;
  S->prev_target->next_target = S;
  Head->prev_target = S;
}

static bool convert_endpoint(const struct sockaddr_storage *from, struct sockaddr_storage *to) {
  if (from->ss_family == AF_INET && inet_sockaddr_address(from) == LOCALHOST) {
    const uint16_t port = inet_sockaddr_port(from);
    const char *path = unix_socket_path(unix_socket_directory, engine_username(), port);

    if (path) {
      struct stat st;
      if (!stat(path, &st) && ((st.st_mode & S_IFMT) == S_IFSOCK)) {
        *to = make_unix_sockaddr_storage(path, port);
        return true;
      }
    }
  }

  return false;
}

static void convert_target(const conn_target_t **source, conn_target_t *converted) {
  if ((*source)->endpoint.ss_family == AF_INET && inet_sockaddr_address(&((*source)->endpoint)) == LOCALHOST) {
    *converted = **source;
    if (convert_endpoint(&(*source)->endpoint, &converted->endpoint)) {
      *source = converted;
    }
  }
}

// `was_created` return value: 1 = created new, 2 = refcnt changed from 0 to 1, 0 = existed before with refcnt > 0.
conn_target_t *create_target(const conn_target_t *source, int *was_created) {

  conn_target_t conn_target_converted;
  convert_target(&source, &conn_target_converted);

  conn_target_t *t = find_target(source, lookup, NULL);
  tvkprintf(net_connections, 4,  "%s: %s t->refcnt %d\n", __func__, sockaddr_storage_to_string(&source->endpoint), t ? t->refcnt : 0);
  if (t) {
    assert(t->refcnt >= 0);
    t->min_connections = source->min_connections;
    t->max_connections = source->max_connections;
    t->reconnect_timeout = source->reconnect_timeout;
    if (!t->refcnt++) {
      remove_target_from_list(t);
      insert_target_into_list(t, &ActiveTargets);
      inactive_targets--;
      active_targets++;
      t->generation++;
      t->next_reconnect = precise_now;
    }
    if (was_created) {
      *was_created = (t->refcnt == 1) ? 2 : 0;
    }
  } else {
    if (free_targets) {
      assert(FreeTargets.next_target != &FreeTargets);
      t = FreeTargets.next_target;
      remove_target_from_list(t);
      free_targets--;
    } else {
      assert(allocated_targets < MAX_TARGETS);
      t = &Targets[allocated_targets++];
    }
    int gen = t->generation + 1;
    memcpy(t, source, sizeof(*source));

    t->first_conn = t->last_conn = (struct connection *)t;
    t->first_query = t->last_query = (struct conn_query *)t;
    t->refcnt = 1;
    t->generation = gen;
    insert_target_into_list(t, &ActiveTargets);
    active_targets++;

    find_target(source, insert, t);

    if (was_created) {
      *was_created = 1;
    }
  }
  return t;
}

static int free_target(conn_target_t *S) {
  assert(S && S->type && !S->refcnt);
  assert(S->first_conn == (struct connection *)S);
  assert(S->first_query == (struct conn_query *)S);

  tvkprintf(net_connections, 4, "Freeing unused target to %s\n", sockaddr_storage_to_string(&S->endpoint));
  assert(S == find_target(S, erase, NULL));

  remove_target_from_list(S);

  int t = S->generation + 1;
  memset(S, 0, sizeof(conn_target_t));
  S->refcnt = -1;
  S->generation = t;

  insert_target_into_list(S, &FreeTargets);
  inactive_targets--;
  free_targets++;

  return 1;
}

int destroy_target(conn_target_t *S) {
  assert(S);
  assert(S->type);
  assert(S->refcnt > 0);
  tvkprintf(net_connections, 4, "%s: %s refcnt %d\n", __func__, sockaddr_storage_to_string(&S->endpoint), S->refcnt);
  if (!--S->refcnt) {
    remove_target_from_list(S);
    insert_target_into_list(S, &InactiveTargets);
    active_targets--;
    inactive_targets++;
    S->generation++;
    clean_unused_target(S);
  }
  return S->refcnt;
}

int clean_unused_target(conn_target_t *S) {
  assert(S);
  assert(S->type);
  if (S->refcnt) {
    return 0;
  }
  if (S->first_conn != (struct connection *)S || S->first_query != (struct conn_query *)S) {
    return 0;
  }
  return free_target(S);
}

struct connection *get_default_target_connection(conn_target_t *target) {
  if (!target || !target->outbound_connections) {
    return NULL;
  }

  struct connection *c = target->first_conn;
  while (true) {
    if (c->type->check_ready(c) == cr_ok) {
      return c;
    }
    if (c == target->last_conn) {
      break;
    }
    c = c->next;
  }

  return NULL;
}

static inline int make_client_socket(struct sockaddr_storage *endpoint) {
  switch (endpoint->ss_family) {
    case AF_INET:
      return client_socket(cast_sockaddr_storage_to_inet(endpoint), 0);
    case AF_INET6:
      return client_socket_ipv6(cast_sockaddr_storage_to_inet6(endpoint), SM_IPV6);
    case AF_UNIX:
      return client_socket_unix(cast_sockaddr_storage_to_unix(endpoint), SM_UNIX);
    default:
      assert(0);
  }

  return -1;
}

struct conn_target_connection_stat {
  uint32_t good;
  uint32_t bad;
  uint32_t stopped;
};
typedef struct conn_target_connection_stat conn_target_connection_stat_t;

static conn_target_connection_stat_t conn_target_connection_stat(conn_target_t *target) {
  conn_target_connection_stat_t stat = {.good = 0, .bad = 0, .stopped = 0};

  for (struct connection *c = target->first_conn; c != (struct connection *) target; c = c->next) {
    const int cr = c->type->check_ready(c);
    switch (cr) {
      case cr_notyet:
      case cr_busy:
        break;
      case cr_ok:
        ++stat.good;
        break;
      case cr_stopped:
        ++stat.stopped;
        break;
      case cr_failed:
        ++stat.bad;
        break;
      default:
        assert(0);
    }
  }

  return stat;
}

int create_new_connections(conn_target_t *S) {
  assert(S->refcnt >= 0);

  const conn_target_connection_stat_t target_conn_stat = conn_target_connection_stat(S);

  int count = 0;

  S->ready_outbound_connections = target_conn_stat.good;
  int need_c = S->min_connections + target_conn_stat.bad + ((target_conn_stat.stopped + 1) >> 1);
  if (need_c > S->max_connections) {
    need_c = S->max_connections;
  }

  if (precise_now < S->next_reconnect && !S->active_outbound_connections) {
    return 0;
  }

  while (S->outbound_connections < need_c) {
    struct sockaddr_storage endpoint = S->endpoint;
    convert_endpoint(&S->endpoint, &endpoint);

    const int cfd = S->type->create_outbound ? S->type->create_outbound(&endpoint) : make_client_socket(&endpoint);

    if (cfd < 0) {
      compute_next_reconnect(S);
      tvkprintf(net_connections, 1, "error connecting to %s: %m\n", sockaddr_storage_to_string(&endpoint));
      return count;
    }
    if (cfd >= MAX_EVENTS || cfd >= MAX_CONNECTIONS) {
      close(cfd);
      compute_next_reconnect(S);
      tvkprintf(net_connections, 1, "out of sockets when connecting to %s\n", sockaddr_storage_to_string(&endpoint));
      return count;
    }

    if (cfd > max_connection) {
      max_connection = cfd;
    }

    event_t *ev = epoll_fd_event(cfd);
    struct connection *c = Connections + cfd;
    memset(c, 0, sizeof(struct connection));
    c->fd = cfd;
    c->ev = ev;
    c->target = S;
    c->generation = ++conn_generation;
    c->flags = C_WANTWR;

    c->flags |= S->type->flags & C_RAWMSG;
    init_connection_buffers(c);

    c->timer.wakeup = conn_timer_wakeup_gateway;
    c->write_timer.wakeup = conn_write_timer_wakeup_gateway;
    c->type = S->type;
    c->extra = S->extra;
    c->basic_type = ct_outbound;
    c->status = conn_connecting;
    c->first_query = c->last_query = (struct conn_query *)c;

    c->local_endpoint.ss_family = endpoint.ss_family;
    c->remote_endpoint = endpoint;
    if (endpoint.ss_family == AF_INET) {
      struct sockaddr_in *self = cast_sockaddr_storage_to_inet(&c->local_endpoint);
      socklen_t self_addrlen = sizeof(c->local_endpoint);
      getsockname(c->fd, (struct sockaddr *)self, &self_addrlen);
    } else if (endpoint.ss_family == AF_INET6) {
      c->flags |= C_IPV6;
      struct sockaddr_in6 *self = cast_sockaddr_storage_to_inet6(&c->local_endpoint);
      socklen_t self_addrlen = sizeof(c->local_endpoint);
      getsockname(c->fd, (struct sockaddr *)self, &self_addrlen);
    } else if (endpoint.ss_family == AF_UNIX) {
      c->flags |= C_UNIX;
      struct sockaddr_un *self = cast_sockaddr_storage_to_unix(&c->local_endpoint);
      socklen_t self_addrlen = sizeof(c->local_endpoint);
      getsockname(c->fd, (struct sockaddr *)self, &self_addrlen);
    } else {
      assert(0);
    }

    char buffer_local[SOCKADDR_STORAGE_BUFFER_SIZE], buffer_remote[SOCKADDR_STORAGE_BUFFER_SIZE];
    tvkprintf(net_connections, 4, "created new outbound connection %s -> %s\n", sockaddr_storage_to_buffer(&c->local_endpoint, buffer_local),
             sockaddr_storage_to_buffer(&c->remote_endpoint, buffer_remote));

    if (c->type->init_outbound(c) >= 0) {
      epoll_sethandler(cfd, 0, server_read_write_gateway, c);
      epoll_insert(cfd, (c->flags & C_WANTRD ? EVT_READ : 0) | (c->flags & C_WANTWR ? EVT_WRITE : 0) | EVT_SPEC);
      outbound_connections++;
      S->outbound_connections++;
      outbound_connections_created++;
      count++;
    } else {
      clean_connection_buffers(c);
      c->basic_type = ct_none;
      close(cfd);
      compute_next_reconnect(S);
      return count;
    }

    struct connection* h = (struct connection *)S;
    c->next = h;
    c->prev = h->prev;
    h->prev->next = c;
    h->prev = c;

    tvkprintf(net_connections, 4, "bind handle %d to %s\n", c->fd, sockaddr_storage_to_buffer(&c->remote_endpoint, buffer_remote));
  }
  return count;
}

double conn_close_intensivity = 0.01;
static double prev_close_time;

int close_some_unneeded_connections() {
  if (prev_close_time == 0) {
    prev_close_time = precise_now;
    return 0;
  }
  double t = conn_close_intensivity * (precise_now - prev_close_time);
  if (t < 0) {
    prev_close_time = precise_now;
    return 0;
  }
  if (t * outbound_connections < 0.005) {
    return 0;
  }
  prev_close_time = precise_now;
  t = exp(-t);

  int res = 0;
  conn_target_t *S, *SN;
  struct connection *c, *c_next;

  for (S = InactiveTargets.next_target; S != &InactiveTargets; S = SN) {
    SN = S->next_target;
    assert(!S->refcnt);
    ++S->refcnt;
    for (c = S->first_conn; c != (struct connection *)S; c = c_next) {
      c_next = c->next;
      if (drand48() > t) {
        fail_connection(c, -17);
        res++;
      }
    }
    --S->refcnt;
    clean_unused_target(S);
  }
  return res;
}

int create_all_outbound_connections() {
  int count = 0;
  get_utime_monotonic();
  close_some_unneeded_connections();
  ready_outbound_connections = ready_targets = 0;
  conn_target_t *S;
  for (S = ActiveTargets.next_target; S != &ActiveTargets; S = S->next_target) {
    assert(S->type && S->refcnt > 0);
    count += create_new_connections(S);
    if (S->ready_outbound_connections) {
      ready_outbound_connections += S->ready_outbound_connections;
      ready_targets++;
    }
  }
  return count;
}

void install_client_connection(conn_target_t *S, int fd) {
  assert(S && fd > -1);

  if(fd >= MAX_EVENTS  || fd >= MAX_CONNECTIONS) {
    //  TODO(d.banschikov): handle error here
  }

  const conn_target_connection_stat_t stat = conn_target_connection_stat(S);
  S->ready_outbound_connections = stat.good;
  assert(S->ready_outbound_connections <= S->max_connections);

  struct connection *c = &Connections[fd];
  event_t *ev = epoll_fd_event(fd);
  memset(c, 0, sizeof(*c));
  c->fd = fd;
  c->ev = ev;
  c->target = S;
  c->generation = ++conn_generation;
  c->flags = C_WANTWR;
  c->flags |= S->endpoint.ss_family == AF_UNIX ? C_UNIX : 0;

  c->flags |= S->type->flags & C_RAWMSG;
  init_connection_buffers(c);

  c->timer.wakeup = conn_timer_wakeup_gateway;
  c->write_timer.wakeup = conn_write_timer_wakeup_gateway;
  c->type = S->type;
  c->extra = S->extra;
  c->basic_type = ct_outbound;
  c->status = conn_connecting;
  c->first_query = c->last_query = (struct conn_query *)c;
  c->remote_endpoint = S->endpoint;

  socklen_t socklen = sizeof(c->local_endpoint);
  assert(!getsockname(fd, (struct sockaddr*) &c->local_endpoint, &socklen));


  assert(c->type->init_outbound(c) >= 0);
  epoll_sethandler(fd, 0, server_read_write_gateway, c);
  epoll_insert(fd, (c->flags & C_WANTRD ? EVT_READ : 0) | (c->flags & C_WANTWR ? EVT_WRITE : 0) | EVT_SPEC);
  outbound_connections++;
  S->outbound_connections++;
  outbound_connections_created++;

  struct connection* h = (struct connection *)S;
  c->next = h;
  c->prev = h->prev;
  h->prev->next = c;
  h->prev = c;
}

int conn_event_wakeup_gateway(event_timer_t *et) {
  struct conn_query *q = container_of(et, struct conn_query, timer);
  tvkprintf(net_connections, 4, "ALARM: awakened pending query %p [%d -> %d]\n", q, q->requester ? q->requester->fd : -1, q->outbound ? q->outbound->fd : -1);
  return q->cq_type->wakeup(q);
}

int insert_conn_query_into_list(struct conn_query *q, struct conn_query *h) {
  q->next = h;
  q->prev = h->prev;
  h->prev->next = q;
  h->prev = q;
  if (q->requester) {
    q->req_generation = q->requester->generation;
    q->requester->pending_queries++;
  }
  q->timer.h_idx = 0;
  q->timer.wakeup = conn_event_wakeup_gateway;
  if (q->timer.wakeup_time > 0) {
    set_timer_operation(&q->timer, "connection query");
    insert_event_timer(&q->timer);
  }
  return 0;
}

int push_conn_query_into_list(struct conn_query *q, struct conn_query *h) {
  q->prev = h;
  q->next = h->next;
  h->next->prev = q;
  h->next = q;
  if (q->requester) {
    q->req_generation = q->requester->generation;
    q->requester->pending_queries++;
  }
  q->timer.h_idx = 0;
  q->timer.wakeup = conn_event_wakeup_gateway;
  if (q->timer.wakeup_time > 0) {
    set_timer_operation(&q->timer, "connection query");
    insert_event_timer(&q->timer);
  }
  return 0;
}

int insert_conn_query(struct conn_query *q) {
  tvkprintf(net_connections, 4, "insert_conn_query(%p)\n", q);
  struct conn_query *h = (struct conn_query *)q->outbound;
  return insert_conn_query_into_list(q, h);
}

int push_conn_query(struct conn_query *q) {
  tvkprintf(net_connections, 4, "push_conn_query(%p)\n", q);
  struct conn_query *h = (struct conn_query *)q->outbound;
  return push_conn_query_into_list(q, h);
}

int delete_conn_query(struct conn_query *q) {
  if (!q->prev && !q->next) {
    tvkprintf(net_connections, 4, "delete_conn_query (%p) at second time\n", q);
    return 0;
  }
  tvkprintf(net_connections, 4, "delete_conn_query (%p)\n", q);
  assert(q->prev && q->next);
  q->next->prev = q->prev;
  q->prev->next = q->next;
  q->next = q->prev = NULL;
  if (q->requester && q->requester->generation == q->req_generation) {
    if (!--q->requester->pending_queries) {
      /* wake up master */
      tvkprintf(net_connections, 4, "socket %d was the last one, waking master %d\n", q->outbound ? q->outbound->fd : -1, q->requester->fd);
      if (!q->requester->ev->in_queue) {
        put_event_into_heap(q->requester->ev);
      }
    }
  }
  q->requester = 0;
  q->outbound = 0;
  if (q->timer.h_idx) {
    remove_event_timer(&q->timer);
  }
  return 0;
}

// arseny30: added for php-engine
int delete_conn_query_from_requester(struct conn_query *q) {
  tvkprintf(net_connections, 4, "delete_conn_query_from_requester (%p)\n", q);
  if (q->requester && q->requester->generation == q->req_generation) {
    if (!--q->requester->pending_queries) {
      /* wake up master */
      tvkprintf(net_connections, 4, "socket %d was the last one, waking master %d\n", q->outbound ? q->outbound->fd : -1, q->requester->fd);
      if (!q->requester->ev->in_queue) {
        put_event_into_heap(q->requester->ev);
      }
    }
  }
  q->requester = 0;
  if (q->timer.h_idx) {
    remove_event_timer(&q->timer);
  }
  return 0;
}

#define SH_SIZE 16

static int SHN;
static long long SH[SH_SIZE + 1];

static void stat_heap_push(long long x) {
  if (SHN < SH_SIZE) {
    int i = ++SHN;
    while (i > 1) {
      int j = (i >> 1);
      if (SH[j] <= x) {
        break;
      }
      SH[i] = SH[j];
      i = j;
    }
    SH[i] = x;
  } else if (x > SH[1]) {
    int i = 1, j = (i << 1);
    while (j <= SHN) {
      if (j < SHN && SH[j + 1] < SH[j]) {
        j++;
      }
      if (x <= SH[j]) {
        break;
      }
      SH[i] = SH[j];
      i = j;
      j <<= 1;
    }
    SH[i] = x;
  }
}

static long long stat_heap_pop() {
  if (!SHN) {
    return -1;
  }
  long long x = SH[1], y = SH[SHN--];
  int i = 1, j = 2;
  while (j <= SHN) {
    if (j < SHN && SH[j + 1] < SH[j]) {
      j++;
    }
    if (y <= SH[j]) {
      break;
    }
    SH[i] = SH[j];
    i = j;
    j <<= 1;
  }
  SH[i] = y;
  return x;
}

void dump_connection_buffers_stats() {
  int s;
  SHN = 0;
  for (s = 1; s <= max_connection; s++) {
    struct connection *c = Connections + s;
    if (c->type && c->status != conn_none) {
      long long t = c->In.total_bytes + c->In.unprocessed_bytes;
      if (t > 0) {
        stat_heap_push((t << 20) + s);
      }
      t = c->Out.total_bytes + c->Out.unprocessed_bytes;
      if (t > 0) {
        stat_heap_push((t << 20) + (1 << 19) + s);
      }
    }
  }
  kprintf("BUFFER STATS\n");
  while (SHN > 0) {
    long long t = stat_heap_pop();
    struct connection *c = &Connections[t & ((1 << 19) - 1)];
    fprintf(stderr, "BUFFER STATS: connection %d (type %p(%s), status %d, flags %02x, peer %s) uses %lld bytes of %s buffers\n", c->fd, c->type,
            c->type ? c->type->title : "", c->status, c->flags, sockaddr_storage_to_string(&c->remote_endpoint), t >> 20, t & (1 << 19) ? "output" : "input");
  }
}

void cond_dump_connection_buffers_stats() {
  static int dump_buff_stats_last;
  if (dump_buff_stats_last != now) {
    dump_buff_stats_last = now;
    dump_connection_buffers_stats();
  }
}

int write_out_chk(struct connection *c, const void *data, int len) {
  int res = write_out(&c->Out, data, len);
  if (res < len && !(c->flags & C_FAILED)) {
    kprintf("error while writing to connection #%d (type %p(%s); in.bytes=%d, out.bytes=%d, tmp.bytes=%d; peer %s): %d bytes written out of %d\n", c->fd,
             c->type, c->type ? c->type->title : "", c->In.total_bytes + c->In.unprocessed_bytes, c->Out.total_bytes + c->Out.unprocessed_bytes,
             c->Tmp ? c->Tmp->total_bytes : 0, sockaddr_storage_to_string(&c->remote_endpoint), res, len);
    fail_connection(c, -666);
    cond_dump_connection_buffers_stats();
    return 0;
  } else {
    return 1;
  }
}

void dump_connection_buffers(struct connection *c) {
  fprintf(stderr, "Dumping buffers of connection %d\nINPUT buffers of %d:\n", c->fd, c->fd);
  dump_buffers(&c->In);
  fprintf(stderr, "OUTPUT buffers of %d:\n", c->fd);
  dump_buffers(&c->Out);
  if (c->Tmp) {
    fprintf(stderr, "TEMP buffers of %d:\n", c->fd);
    dump_buffers(c->Tmp);
  }
  fprintf(stderr, "--- END (dumping buffers of connection %d) ---\n", c->fd);
}

