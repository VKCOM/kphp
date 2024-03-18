// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_REACTOR_H
#define KDB_NET_NET_REACTOR_H

#include <sys/cdefs.h>
#include <sys/epoll.h>

#include <stdbool.h>

#define EVT_READ 4
#define EVT_WRITE 2
#define EVT_SPEC 1
#define EVT_RW (EVT_READ | EVT_WRITE)
#define EVT_RWX (EVT_READ | EVT_WRITE | EVT_SPEC)
#define EVT_LEVEL 8
#define EVT_OPEN 0x80
#define EVT_CLOSED 0x40
#define EVT_IN_EPOLL 0x20
#define EVT_NEW 0x100
#define EVT_NOHUP 0x200
#define EVT_FROM_EPOLL 0x400
#define EVT_FAKE 0x800

#define EVA_CONTINUE 0
#define EVA_RERUN -2
#define EVA_REMOVE -3
#define EVA_DESTROY -5
#define EVA_ERROR -8
#define EVA_FATAL -666

typedef void (*epoll_func_vector_t)();
extern epoll_func_vector_t epoll_pre_runqueue, epoll_post_runqueue, epoll_pre_event;

typedef struct event_descr event_t;
typedef int (*event_handler_t)(int fd, void *data, event_t *ev);

struct event_descr {
  int fd;
  int state;       // actions that we should wait for (read/write/special) + status
  int ready;       // actions we are ready to do
  int epoll_ready; // result of epoll()
  int priority;    // priority (0-9)
  int in_queue;    // position in heap (0=not in queue)
  long long timestamp;
  event_handler_t work;
  void *data;
  //  struct sockaddr_in peer;
};

typedef struct event_timer event_timer_t;

typedef int (*event_timer_wakeup_t)(event_timer_t *et);
struct event_timer {
  int h_idx;
  event_timer_wakeup_t wakeup;
  double wakeup_time;
  const char *operation;
};

struct net_reactor_ctx {
  int epoll_fd;
  int max_events;
  int max_timers;
  int event_heap_size;
  int timer_heap_size;
  int now;
  int prev_now;
  int64_t timestamp;
  struct epoll_event *epoll_events;
  event_t *events;
  event_t *timers;
  event_t **event_heap;
  event_timer_t **timer_heap;
  epoll_func_vector_t pre_runqueue;
  epoll_func_vector_t post_runqueue;
  epoll_func_vector_t pre_event;
  double wait_start;
  double last_wait;
  double total_idle_time;
  double average_idle_time;
  double average_idle_quotient;
};
typedef struct net_reactor_ctx net_reactor_ctx_t;

static inline int net_reactor_events(const net_reactor_ctx_t *reactor_ctx) {
  return reactor_ctx->event_heap_size;
}

static inline int net_reactor_timers(const net_reactor_ctx_t *reactor_ctx) {
  return reactor_ctx->timer_heap_size;
}

void net_reactor_alloc(net_reactor_ctx_t *ctx, int max_events, int max_timers);
void net_reactor_free(net_reactor_ctx_t *ctx);
bool net_reactor_init(net_reactor_ctx_t *ctx);
bool net_reactor_create(net_reactor_ctx_t *ctx, int max_events, int max_timers);
void net_reactor_destroy(net_reactor_ctx_t *ctx);
void net_reactor_clear_event(net_reactor_ctx_t *ctx, int fd);
event_t *net_reactor_fd_event(net_reactor_ctx_t *ctx, int fd);
int net_reactor_put_event_into_heap(net_reactor_ctx_t *ctx, event_t *ev);
int net_reactor_put_event_into_heap_tail(net_reactor_ctx_t *ctx, event_t *ev, int ts_delta);
void net_reactor_set_handler(net_reactor_ctx_t *ctx, int fd, int prio, event_handler_t handler, void *data);
int net_reactor_wait(net_reactor_ctx_t *ctx, int timeout);
void net_reactor_fetch_events(net_reactor_ctx_t *ctx, int num_events);

int net_reactor_runqueue(net_reactor_ctx_t *ctx);
int net_reactor_run_timers(net_reactor_ctx_t *ctx);
int net_reactor_work_timers(net_reactor_ctx_t *ctx, int timeout);
void net_reactor_sleep_before_wait(net_reactor_ctx_t *ctx);
int net_reactor_insert(net_reactor_ctx_t *ctx, int fd, int flags);
int net_reactor_remove(net_reactor_ctx_t *ctx, int fd);
int net_reactor_close(net_reactor_ctx_t *ctx, int fd);
int net_reactor_insert_event_timer(net_reactor_ctx_t *ctx, event_timer_t *et);
int net_reactor_remove_event_timer(net_reactor_ctx_t *ctx, event_timer_t *et);
bool net_reactor_has_too_many_timers(net_reactor_ctx_t *ctx);
void net_reactor_update_timer_counters(net_reactor_ctx_t *ctx, int timeout);

int net_reactor_work(net_reactor_ctx_t *ctx, int timeout);

#endif // KDB_NET_NET_REACTOR_H
