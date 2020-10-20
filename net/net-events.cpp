#include "net/net-events.h"

#include "common/precise-time.h"

net_reactor_ctx_t main_thread_reactor = {.epoll_fd = -1,
                                         .max_events = 0,
                                         .max_timers = 0,
                                         .event_heap_size = 0,
                                         .timer_heap_size = 0,
                                         .now = 0,
                                         .prev_now = 0,
                                         .timestamp = 0,
                                         .epoll_events = NULL,
                                         .events = NULL,
                                         .timers = NULL,
                                         .event_heap = NULL,
                                         .timer_heap = NULL,
                                         .pre_runqueue = NULL,
                                         .post_runqueue = NULL,
                                         .pre_event = NULL,
                                         .wait_start = 0,
                                         .last_wait = 0,
                                         .total_idle_time = 0,
                                         .average_idle_time = 0,
                                         .average_idle_quotient = 0};

static void main_thread_reactor_alloc(void) __attribute__((constructor));

static void main_thread_reactor_alloc(void) {
  net_reactor_alloc(&main_thread_reactor, MAX_EVENTS, MAX_EVENT_TIMERS);
}
