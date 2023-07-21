// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-reactor.h"

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/kernel-version.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/parallel/thread-id.h"
#include "common/precise-time.h"
#include "common/server/signals.h"

#include "net/net-msg-buffers.h"
#include "net/time-slice.h"

DEFINE_VERBOSITY(net_events);

static int epoll_sleep_time;
static const double max_time_slice = 0.05;

OPTION_PARSER(OPT_NETWORK, "epoll-sleep-time", required_argument, "sleep time in main cycle, set in microseconds (between 1mcs and 0.5s), experimental") {
  epoll_sleep_time = atoi(optarg);
  assert(0 < epoll_sleep_time && epoll_sleep_time <= 500000);
  return 0;
}

void net_reactor_alloc(net_reactor_ctx_t *ctx, int max_events, int max_timers) {
  ctx->max_events = max_events;
  ctx->max_timers = max_timers;
  ctx->timestamp = 0;
  ctx->event_heap_size = 0;
  ctx->timer_heap_size = 0;
  ctx->now = 0;
  ctx->prev_now = 0;
  ctx->events = static_cast<event_t*>(calloc(max_events, sizeof(ctx->events[0])));
  ctx->event_heap = static_cast<event_t**>(calloc(max_events + 1, sizeof(ctx->event_heap[0])));
  ctx->timer_heap = static_cast<event_timer_t**>(calloc(max_timers + 1, sizeof(ctx->timer_heap[0])));
  ctx->epoll_events = static_cast<epoll_event*>(calloc(max_events, sizeof(ctx->epoll_events[0])));
  ctx->pre_runqueue = ctx->post_runqueue = ctx->pre_event = NULL;
  ctx->wait_start = 0;
  ctx->last_wait = 0;
  ctx->total_idle_time = 0;
  ctx->average_idle_time = 0;
  ctx->average_idle_quotient = 0;
}

void net_reactor_free(net_reactor_ctx_t *ctx) {
  free(ctx->events);
  free(ctx->event_heap);
  free(ctx->timer_heap);
  free(ctx->epoll_events);
}

bool net_reactor_init(net_reactor_ctx_t *ctx) {
  ctx->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (ctx->epoll_fd >= 0) {
    return true;
  }

  return false;
}

bool net_reactor_create(net_reactor_ctx_t *ctx, int max_events, int max_timers) {
  ctx->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (ctx->epoll_fd >= 0) {
    net_reactor_alloc(ctx, max_events, max_timers);

    return true;
  }

  tvkprintf(net_events, 0, "epoll_create(): %m\n");

  return false;
}

void net_reactor_destroy(net_reactor_ctx_t *ctx) {
  close(ctx->epoll_fd);
}

event_t *net_reactor_fd_event(net_reactor_ctx_t *ctx, int fd) {
  assert(0 <= fd && fd < ctx->max_events);

  return &ctx->events[fd];
}

/* returns positive value if x is greater than y */
/* since we use only "greater_ev(x,y) > 0" and "greater_ev(x,y) <= 0" compares, */
/* it is unimportant to distinguish "x<y" and "x==y" cases */
static int event_comparator(const event_t *x, const event_t *y) {
  const int priority = x->priority - y->priority;
  if (priority)
    return priority;
  return (x->timestamp > y->timestamp) ? 1 : 0;
}

static event_t *net_reactor_pop_event_heap_head(net_reactor_ctx_t *ctx) {
  int i, j, N = ctx->event_heap_size;
  event_t *x, *y;
  if (!N)
    return 0;
  event_t *ev = ctx->event_heap[1];
  assert(ev && ev->in_queue == 1);
  ev->in_queue = 0;
  if (!--ctx->event_heap_size)
    return ev;
  x = ctx->event_heap[N--];
  i = 1;
  while (true) {
    j = (i << 1);
    if (j > N)
      break;
    if (j < N && event_comparator(ctx->event_heap[j], ctx->event_heap[j + 1]) > 0)
      j++;
    y = ctx->event_heap[j];
    if (event_comparator(x, y) <= 0)
      break;
    ctx->event_heap[i] = y;
    y->in_queue = i;
    i = j;
  }
  ctx->event_heap[i] = x;
  x->in_queue = i;
  return ev;
}

static int net_reactor_remove_event_from_heap(net_reactor_ctx_t *ctx, event_t *ev, bool allow_hole) {
  int v = ev->fd, i, j, N = ctx->event_heap_size;
  event_t *x;
  assert(v >= 0 && v < ctx->max_events && ctx->events + v == ev);
  i = ev->in_queue;
  if (!i) {
    return 0;
  }
  assert(i > 0 && i <= N);
  ev->in_queue = 0;
  do {
    j = (i << 1);
    if (j > N)
      break;
    if (j < N && event_comparator(ctx->event_heap[j], ctx->event_heap[j + 1]) > 0)
      j++;
    ctx->event_heap[i] = x = ctx->event_heap[j];
    x->in_queue = i;
    i = j;
  } while (true);
  if (allow_hole) {
    ctx->event_heap[i] = NULL;
    return i;
  }
  if (i < N) {
    ev = ctx->event_heap[N];
    ctx->event_heap[N] = 0;
    while (i > 1) {
      j = (i >> 1);
      x = ctx->event_heap[j];
      if (event_comparator(x, ev) <= 0)
        break;
      ctx->event_heap[i] = x;
      x->in_queue = i;
      i = j;
    }
    ctx->event_heap[i] = ev;
    ev->in_queue = i;
  }
  ctx->event_heap_size--;
  return N;
}

void net_reactor_clear_event(net_reactor_ctx_t *ctx, int fd) {
  assert(0 <= fd && fd < ctx->max_events);

  event_t *event = &ctx->events[fd];
  if (event->in_queue) {
    net_reactor_remove_event_from_heap(ctx, event, false);
  }
  memset(event, 0, sizeof(*event));
}

int net_reactor_put_event_into_heap(net_reactor_ctx_t *ctx, event_t *ev) {
  int v = ev->fd, i, j;
  assert(v >= 0 && v < ctx->max_events && ctx->events + v == ev);
  i = ev->in_queue ? net_reactor_remove_event_from_heap(ctx, ev, true) : ++ctx->event_heap_size;
  assert(i <= ctx->max_events);

  while (i > 1) {
    j = (i >> 1);
    event_t *x = ctx->event_heap[j];
    if (event_comparator(x, ev) <= 0)
      break;
    ctx->event_heap[i] = x;
    x->in_queue = i;
    i = j;
  }
  ctx->event_heap[i] = ev;
  ev->in_queue = i;
  return i;
}

int net_reactor_put_event_into_heap_tail(net_reactor_ctx_t *ctx, event_t *ev, int ts_delta) {
  ev->timestamp = ctx->timestamp + ts_delta;
  return net_reactor_put_event_into_heap(ctx, ev);
}

void net_reactor_set_handler(net_reactor_ctx_t *ctx, int fd, int prio, event_handler_t handler, void *data) {
  assert(0 <= fd && fd < ctx->max_events);

  event_t *event = &ctx->events[fd];
  if (event->fd != fd) {
    memset(event, 0, sizeof(*event));
    event->fd = fd;
  }
  event->priority = prio;
  event->data = data;
  event->work = handler;
}

static int epoll_conv_flags(int flags) {
  int r = EPOLLERR;
  if (flags == 0x204) {
    return EPOLLIN;
  }
  if (!flags) {
    return 0;
  }
  if (!(flags & EVT_NOHUP)) {
    r |= EPOLLHUP;
  }
  if (flags & EVT_READ) {
    r |= EPOLLIN;
  }
  if (flags & EVT_WRITE) {
    r |= EPOLLOUT;
  }
  if (flags & EVT_SPEC) {
    r |= EPOLLRDHUP | EPOLLPRI;
  }
  if (!(flags & EVT_LEVEL)) {
    r |= EPOLLET;
  }
  if (flags & EPOLLONESHOT) {
    r |= EPOLLONESHOT;
  }
  return r;
}

static int epoll_unconv_flags(int f) {
  int r = EVT_FROM_EPOLL;
  if (f & (EPOLLIN | EPOLLERR)) {
    r |= EVT_READ;
  }
  if (f & EPOLLOUT) {
    r |= EVT_WRITE;
  }
  if (f & (EPOLLRDHUP | EPOLLPRI)) {
    r |= EVT_SPEC;
  }
  return r;
}

int net_reactor_wait(net_reactor_ctx_t *ctx, int timeout) {
  return epoll_wait(ctx->epoll_fd, ctx->epoll_events, ctx->max_events, timeout);
}

void net_reactor_fetch_events(net_reactor_ctx_t *ctx, int num_events) {
  for (int i = 0; i < num_events; ++i) {
    const int fd = ctx->epoll_events[i].data.fd;
    assert(0 <= fd && fd < ctx->max_events);
    event_t *event = &ctx->events[fd];
    assert(event->fd == fd);
    event->epoll_ready = ctx->epoll_events[i].events;
    event->ready |= epoll_unconv_flags(event->epoll_ready = ctx->epoll_events[i].events);
    event->timestamp = ctx->timestamp;
    net_reactor_put_event_into_heap(ctx, event);
  }

  ctx->last_wait = get_utime_monotonic();
}

void net_reactor_update_timer_counters(net_reactor_ctx_t *ctx, int timeout) {
  const double wait_time = ctx->last_wait - ctx->wait_start;
  if (wait_time > (timeout / 1000.0 + 0.5)) {
    kprintf("epoll-wait worked too long: %.3fs\n", wait_time);
  }
  ctx->total_idle_time += wait_time;
  ctx->average_idle_time += wait_time;
  ctx->now = time(0);
  if (ctx->now > ctx->prev_now && ctx->now < ctx->prev_now + 60) {
    while (ctx->prev_now < ctx->now) {
      ctx->average_idle_time *= 100.0 / 101;
      ctx->average_idle_quotient = ctx->average_idle_quotient * (100.0 / 101) + 1;
      ctx->prev_now++;
    }
  } else {
    ctx->prev_now = ctx->now;
  }
}

#ifndef EPOLLEXCLUSIVE
#define EPOLLEXCLUSIVE (1u << 28)
#endif

int net_reactor_insert(net_reactor_ctx_t *ctx, int fd, int flags) {
  event_t *ev;
  int ef;
  struct epoll_event ee;
  memset(&ee, 0, sizeof(ee));
  if (!flags) {
    return net_reactor_remove(ctx, fd);
  }
  assert(fd >= 0 && fd < ctx->max_events);
  ev = ctx->events + fd;
  if (ev->fd != fd) {
    memset(ev, 0, sizeof(event_t));
    ev->fd = fd;
  }

  if (ev->state & EVT_FAKE) {
    return 0;
  }

  flags &= EVT_NEW | EVT_NOHUP | EVT_LEVEL | EVT_RWX | EPOLLONESHOT;
  ev->ready = 0; // !!! this bugfix led to some AIO-related bugs, now fixed with the aid of C_REPARSE flag
  if ((ev->state & (EVT_LEVEL | EVT_RWX | EVT_IN_EPOLL)) == flags + EVT_IN_EPOLL) {
    return 0;
  }
  ev->state = (ev->state & ~(EVT_LEVEL | EVT_RWX)) | (flags & (EVT_LEVEL | EVT_RWX));
  ef = epoll_conv_flags(flags);
  if (ef || (flags & EVT_NEW) || !(ev->state & EVT_IN_EPOLL)) {
    ee.events = ef;
    if (epoll_exclusive_supported() && (ef & ~(EPOLLIN | EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLERR)) == 0) {
      ee.events |= EPOLLEXCLUSIVE;
    }
    ee.data.fd = fd;

    tvkprintf(net_events, 3, "epoll_ctl(%d,%d,%d,%d,%08x)\n", ctx->epoll_fd, (ev->state & EVT_IN_EPOLL) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, fd, ee.data.fd,
              ee.events);

    if (epoll_ctl(ctx->epoll_fd, (ev->state & EVT_IN_EPOLL) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, fd, &ee) < 0) {
#if defined(__APPLE__)
      // TODO understand why
      if (errno != ENOENT)
#endif
      tvkprintf(net_events, 0, "epoll_ctl(): %m\n");
    }
    ev->state |= EVT_IN_EPOLL;
  }
  return 0;
}

int net_reactor_remove(net_reactor_ctx_t *ctx, int fd) {
  assert(fd >= 0 && fd < ctx->max_events);

  event_t *ev = ctx->events + fd;
  if (ev->fd != fd) {
    return -1;
  }

  if (!(ev->state & EVT_FAKE) && (ev->state & EVT_IN_EPOLL)) {
    ev->state &= ~EVT_IN_EPOLL;
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, fd, 0) < 0) {
      tvkprintf(net_events, 0, "epoll_ctl(): %m\n");
    }
  }

  return 0;
}

int net_reactor_close(net_reactor_ctx_t *ctx, int fd) {
  assert(fd >= 0 && fd < ctx->max_events);

  event_t *ev = ctx->events + fd;
  if (ev->fd != fd) {
    return -1;
  }
  net_reactor_remove(ctx, fd);
  if (ev->in_queue) {
    net_reactor_remove_event_from_heap(ctx, ev, false);
  }
  memset(ev, 0, sizeof(event_t));
  ev->fd = -1;
  return 0;
}

static inline int basic_et_adjust(net_reactor_ctx_t *ctx, event_timer_t *et, int i) {
  int j;
  while (i > 1) {
    j = (i >> 1);
    if (ctx->timer_heap[j]->wakeup_time <= et->wakeup_time) {
      break;
    }
    ctx->timer_heap[i] = ctx->timer_heap[j];
    ctx->timer_heap[i]->h_idx = i;
    i = j;
  }
  j = 2 * i;
  while (j <= ctx->timer_heap_size) {
    if (j < ctx->timer_heap_size && ctx->timer_heap[j]->wakeup_time > ctx->timer_heap[j + 1]->wakeup_time) {
      j++;
    }
    if (et->wakeup_time <= ctx->timer_heap[j]->wakeup_time) {
      break;
    }
    ctx->timer_heap[i] = ctx->timer_heap[j];
    ctx->timer_heap[i]->h_idx = i;
    i = j;
    j <<= 1;
  }
  ctx->timer_heap[i] = et;
  et->h_idx = i;
  return i;
}

static int event_timer_cmp(const void *l, const void *r) {
  event_timer_t *left = *(event_timer_t **) l;
  event_timer_t *right = *(event_timer_t **) r;
  if (left->operation == NULL || right->operation == NULL) {
    if (left->operation == NULL && right->operation == NULL) {
      return 0;
    }
    return left->operation == NULL ? -1 : 1;
  }
  return strcmp(left->operation, right->operation);
}

static void dump_too_many_event_timers(net_reactor_ctx_t *ctx) {
  tvkprintf(net_events, 0, "Too many event timers: %d\n", ctx->timer_heap_size);
  qsort(&ctx->timer_heap[1], (size_t) ctx->timer_heap_size, sizeof(ctx->timer_heap[0]), event_timer_cmp);
  for (int i = 1; i <= ctx->timer_heap_size;) {
    int j = i;
    while (j != ctx->timer_heap_size + 1 && event_timer_cmp(&ctx->timer_heap[i], &ctx->timer_heap[j]) == 0) {
      j++;
    }
    tvkprintf(net_events, 0, "%d * %s\n", j - i, ctx->timer_heap[i]->operation);
    i = j;
  }
}

bool net_reactor_has_too_many_timers(net_reactor_ctx_t *ctx) {
  return ctx->timer_heap_size * 2 >= ctx->max_timers;
}

int net_reactor_insert_event_timer(net_reactor_ctx_t *ctx, event_timer_t *et) {
  int i;
  if (et->h_idx) {
    i = et->h_idx;
    assert(i > 0 && i <= ctx->timer_heap_size && ctx->timer_heap[i] == et);
  } else {
    if (ctx->timer_heap_size >= ctx->max_timers) {
      dump_too_many_event_timers(ctx);
    }
    assert(ctx->timer_heap_size < ctx->max_timers);
    i = ++ctx->timer_heap_size;
  }

  return basic_et_adjust(ctx, et, i);
}

int net_reactor_remove_event_timer(net_reactor_ctx_t *ctx, event_timer_t *et) {
  int i = et->h_idx;
  if (!i) {
    return 0;
  }
  assert(i > 0 && i <= ctx->timer_heap_size && ctx->timer_heap[i] == et);
  et->h_idx = 0;

  et = ctx->timer_heap[ctx->timer_heap_size--];
  if (i > ctx->timer_heap_size) {
    return 1;
  }
  basic_et_adjust(ctx, et, i);
  return 1;
}

int net_reactor_run_timers(net_reactor_ctx_t *ctx) {
  double wait_time;
  event_timer_t *et;
  if (!ctx->timer_heap_size) {
    return 100000;
  }
  wait_time = ctx->timer_heap[1]->wakeup_time - precise_now;
  if (wait_time > 0) {
    // do not remove this useful debug!
    tvkprintf(net_events, 4, "%d event timers, next in %.3f seconds\n", ctx->timer_heap_size, wait_time);
    return (int)(std::min(100.0, wait_time) * 1000) + 1; // min to prevent integer overflow
  }

  const vk::net::TimeSlice time_slice(max_time_slice);
  while (ctx->timer_heap_size > 0 && ctx->timer_heap[1]->wakeup_time <= precise_now && !pending_signals && !time_slice.expired()) {
    et = ctx->timer_heap[1];
    assert(et->h_idx == 1);
    net_reactor_remove_event_timer(ctx, et);
    et->wakeup(et);
  }
  return 0;
}

int net_reactor_runqueue(net_reactor_ctx_t *ctx) {
  event_t *ev;
  int res, cnt = 0;
  if (!ctx->event_heap_size) {
    return 0;
  }

  tvkprintf(net_events, 3, "epoll_runqueue: %d events\n", ctx->event_heap_size);

  if (ctx->pre_runqueue) {
    (*ctx->pre_runqueue)();
  }
  ctx->timestamp += 2;
  const vk::net::TimeSlice time_slice(max_time_slice);
  while (ctx->event_heap_size && (ev = ctx->event_heap[1])->timestamp < ctx->timestamp && !pending_signals && !time_slice.expired()) {
    net_reactor_pop_event_heap_head(ctx);
    const int fd = ev->fd;
    assert(ev == ctx->events + fd && fd >= 0 && fd < ctx->max_events);
    if (ev->work) {
      if (ctx->pre_event) {
        (*ctx->pre_event)();
      }
      res = ev->work(fd, ev->data, ev);
    } else {
      res = EVA_REMOVE;
    }
    if (res == EVA_REMOVE || res == EVA_DESTROY || res <= EVA_ERROR) {
      net_reactor_remove_event_from_heap(ctx, ev, false);
      net_reactor_remove(ctx, ev->fd);
      if (res == EVA_DESTROY) {
        if (!(ev->state & EVT_CLOSED)) {
          close(ev->fd);
        }
        memset(ev, 0, sizeof(event_t));
      }
      if (res <= EVA_FATAL) {
        tvkprintf(net_events, 0, "fatal: %m\n");
        exit(1);
      }
    } else if (res == EVA_RERUN) {
      ev->timestamp = ctx->timestamp;
      net_reactor_put_event_into_heap(ctx, ev);
    } else if (res > 0) {
      net_reactor_insert(ctx, fd, res & (EVT_LEVEL | EVT_RWX));
    } else if (res == EVA_CONTINUE) {
      ev->ready = 0;
    }
    cnt++;
  }
  if (ctx->post_runqueue) {
    (*ctx->post_runqueue)();
  }
  return cnt;
}

int net_reactor_work_timers(net_reactor_ctx_t *ctx, int timeout) {
  if (ctx->event_heap_size || ctx->timer_heap_size) {
    ctx->now = time(0);
    get_utime_monotonic();
    const vk::net::TimeSlice time_slice(max_time_slice);
    do {
      net_reactor_runqueue(ctx);
      timeout = net_reactor_run_timers(ctx);
    } while ((timeout <= 0 || ctx->event_heap_size) && !pending_signals && !time_slice.expired());
  }
  if (pending_signals) {
    return 0;
  }

  return timeout;
}

void net_reactor_sleep_before_wait(net_reactor_ctx_t *ctx) {
  ctx->wait_start = get_utime_monotonic();

  if (epoll_sleep_time) {
    struct timespec epoll_sleep_time_str;
    epoll_sleep_time_str.tv_sec = 0;
    epoll_sleep_time_str.tv_nsec = (epoll_sleep_time + (lrand48() % epoll_sleep_time)) * 1000;
    nanosleep(&epoll_sleep_time_str, NULL);
  }
}

int net_reactor_work(net_reactor_ctx_t *ctx, int timeout) {
  int timeout2 = 10000;
  timeout2 = net_reactor_work_timers(ctx, timeout2);

  net_reactor_sleep_before_wait(ctx);
  const int min_timeout = timeout < timeout2 ? timeout : timeout2;

  int events = net_reactor_wait(ctx, min_timeout);
  if (events < 0 && errno == EINTR) {
    events = 0;
  }
  if (events < 0) {
    tvkprintf(net_events, 0, "epoll_wait(): %m\n");
  }
  if (verbosity > 1 && events) {
    tvkprintf(net_events, 0, "epoll_wait(%d, ...) = %d\n", ctx->epoll_fd, events);
  }

  net_reactor_fetch_events(ctx, events);

  net_reactor_update_timer_counters(ctx, min_timeout);

  net_reactor_run_timers(ctx);
  return net_reactor_runqueue(ctx);
}
