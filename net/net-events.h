// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VK_NET_EVENTS_H__
#define __VK_NET_EVENTS_H__

#include <assert.h>
#include <errno.h>

#include "common/kprintf.h"
#include "common/precise-time.h"

#include "net/net-reactor.h"

#define        MAX_EVENTS                (1 << 19)
#define        MAX_EVENT_TIMERS        (1 << 19)

DECLARE_VERBOSITY(net_events);

extern net_reactor_ctx_t main_thread_reactor;

static inline void set_timer_params(event_timer_t *timer, event_timer_wakeup_t wakeup, double wakeup_time, const char *operation) {
  timer->wakeup = wakeup;
  timer->wakeup_time = wakeup_time;
  timer->operation = operation;
}

static inline void set_timer_operation(event_timer_t *timer, const char *operation) {
  timer->operation = operation;
}

static inline int epoll_event_heap_size() {
  return net_reactor_events(&main_thread_reactor);
}

static inline int epoll_timer_heap_size() {
  return net_reactor_timers(&main_thread_reactor);
}

static inline int epoll_fd() {
  return main_thread_reactor.epoll_fd;
}

static inline event_t* epoll_fd_event(int fd) {
  return net_reactor_fd_event(&main_thread_reactor, fd);
}

static inline double epoll_total_idle_time() {
  return main_thread_reactor.total_idle_time;
}

static inline double epoll_average_idle_time() {
  return main_thread_reactor.average_idle_time;
}

static inline double epoll_average_idle_quotient() {
  return main_thread_reactor.average_idle_quotient;
}

static inline void init_epoll() {
  if (main_thread_reactor.epoll_fd == -1) {
    const bool ok = net_reactor_init(&main_thread_reactor);
    assert(ok);
  }
}

static inline void close_epoll() {
  net_reactor_destroy(&main_thread_reactor);
  main_thread_reactor.epoll_fd = -1;
}

static inline void clear_event(int event) {
  net_reactor_clear_event(&main_thread_reactor, event);
}

static inline int put_event_into_heap(event_t *ev) {
  return net_reactor_put_event_into_heap(&main_thread_reactor, ev);
}

static inline int put_event_into_heap_tail(event_t *ev, int ts_delta) {
  return net_reactor_put_event_into_heap_tail(&main_thread_reactor, ev, ts_delta);
}

static inline void epoll_sethandler(int fd, int prio, event_handler_t handler, void *data) {
  return net_reactor_set_handler(&main_thread_reactor, fd, prio, handler, data);
}

static inline int epoll_fetch_events(int timeout) {
  int events = net_reactor_wait(&main_thread_reactor, timeout);
  if (events < 0 && errno == EINTR) {
    events = 0;
  }
  if (events < 0) {
    kprintf("epoll_wait(): %m\n");
  }
  net_reactor_fetch_events(&main_thread_reactor, events);

  return events;
}

static inline int epoll_work(int timeout) {
  int timeout2 = 10000;
  timeout2 = net_reactor_work_timers(&main_thread_reactor, timeout2);

  net_reactor_sleep_before_wait(&main_thread_reactor);

  const int min_timeout = timeout < timeout2 ? timeout : timeout2;
  epoll_fetch_events(min_timeout);

  net_reactor_update_timer_counters(&main_thread_reactor, min_timeout);
  now = main_thread_reactor.now;

  net_reactor_run_timers(&main_thread_reactor);
  return net_reactor_runqueue(&main_thread_reactor);

}

static inline int epoll_insert(int fd, int flags) {
  return net_reactor_insert(&main_thread_reactor, fd, flags);
}

static inline int epoll_remove(int fd) {
  return net_reactor_remove(&main_thread_reactor, fd);
}

static inline int epoll_close(int fd) {
  return net_reactor_close(&main_thread_reactor, fd);
}

static inline bool is_too_many_timers() {
  return net_reactor_has_too_many_timers(&main_thread_reactor);
}

static inline int insert_event_timer(event_timer_t *et) {
  return net_reactor_insert_event_timer(&main_thread_reactor, et);
}

static inline int remove_event_timer(event_timer_t *et) {
  return net_reactor_remove_event_timer(&main_thread_reactor, et);
}

static inline bool event_timer_active(const event_timer_t *et) {
  return et->h_idx;
}

#endif
