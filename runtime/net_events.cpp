// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/net_events.h"

#include "common/precise-time.h"
#include "common/wrappers/overloaded.h"

#include "runtime/allocator.h"
#include "runtime/curl.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/rpc.h"
#include "server/curl-adaptor.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/response.h"
#include "server/php-queries.h"

int timeout_convert_to_ms(double timeout) {
  int timeout_ms = (int)(timeout * 1000 + 1);
  if (timeout_ms <= 0) {
    timeout_ms = 1;
  }
  if (timeout_ms >= MAX_TIMEOUT_MS) {
    timeout_ms = MAX_TIMEOUT_MS;
  }

  return timeout_ms;
}


static double kphp_precise_now;

void update_precise_now() {
  kphp_precise_now = get_network_time();
}

double get_precise_now() {
  return kphp_precise_now;
}

bool process_net_event(net_event_t *e) {
  if (e == nullptr) {
    return false;
  }

  std::visit(overloaded {
     [&](const net_events_data::rpc_answer &data) {
         process_rpc_answer(e->slot_id, data.result, data.result_len);
     },
     [&](const net_events_data::rpc_error &data) {
         process_rpc_error(e->slot_id, data.error_code, data.error_message);
     },
     [&](const net_events_data::job_worker_answer &data) {
         process_job_answer(e->slot_id, data.job_result);
     },
     [&](database_drivers::Response *response) {
         php_assert(e->slot_id == response->bound_request_id);
         vk::singleton<database_drivers::Adaptor>::get().process_external_db_response_event(std::unique_ptr<database_drivers::Response>(response));
     },
     [&](curl_async::CurlResponse *response) {
         php_assert(e->slot_id == response->bound_request_id);
         vk::singleton<curl_async::CurlAdaptor>::get().process_response_event(std::unique_ptr<curl_async::CurlResponse>(response));
     }
    }, e->data);

  return true;
}

static int process_net_events() {
  int result = 0;
  while (process_net_event(pop_net_event())) {
    result++;
  }
  return result;
}


const int MAX_WAKEUP_CALLBACKS_EXP = 3;
const int EVENT_TIMERS_HEAP_INDEX_MASK = ((1 << (31 - MAX_WAKEUP_CALLBACKS_EXP)) - 1);
static void (*wakeup_callbacks[1 << MAX_WAKEUP_CALLBACKS_EXP ])(kphp_event_timer *timer);
static int wakeup_callbacks_size;

int register_wakeup_callback(void (*wakeup)(kphp_event_timer *timer)) {
  php_assert (dl::query_num == 0);
  php_assert (wakeup != nullptr);
  php_assert (wakeup_callbacks_size < (1 << MAX_WAKEUP_CALLBACKS_EXP));
  wakeup_callbacks[wakeup_callbacks_size] = wakeup;
  return wakeup_callbacks_size++;
}

static kphp_event_timer **event_timers_heap;
static int event_timers_heap_size;
static int event_timers_max_heap_size;

static inline int event_timer_heap_move_up(double wakeup_time, int i) {
  while (i > 1) {
    int j = (i >> 1);
    if (event_timers_heap[j]->wakeup_time <= wakeup_time) {
      break;
    }
    event_timers_heap[i] = event_timers_heap[j];
    event_timers_heap[i]->heap_index ^= i ^ j;
    i = j;
  }
  return i;
}

static inline int event_timer_heap_move_down(double wakeup_time, int i) {
  int j = 2 * i;
  while (j <= event_timers_heap_size) {
    if (j < event_timers_heap_size && event_timers_heap[j]->wakeup_time > event_timers_heap[j + 1]->wakeup_time) {
      j++;
    }
    if (wakeup_time <= event_timers_heap[j]->wakeup_time) {
      break;
    }
    event_timers_heap[i] = event_timers_heap[j];
    event_timers_heap[i]->heap_index ^= i ^ j;
    i = j;
    j <<= 1;
  }
  return i;
}

kphp_event_timer *allocate_event_timer(double wakeup_time, int wakeup_callback_id, int wakeup_extra) {
  kphp_event_timer *et = static_cast <kphp_event_timer *> (dl::allocate(sizeof(kphp_event_timer)));
  php_assert (0 <= wakeup_callback_id && wakeup_callback_id < wakeup_callbacks_size);
  php_assert (get_precise_now() < wakeup_time);

  et->wakeup_extra = wakeup_extra;
  et->wakeup_time = wakeup_time;

  int i = ++event_timers_heap_size;
  if (i == event_timers_max_heap_size) {
    event_timers_heap = static_cast <kphp_event_timer **> (dl::reallocate(event_timers_heap, sizeof(kphp_event_timer *) * 2 * event_timers_max_heap_size, sizeof(kphp_event_timer *) * event_timers_max_heap_size));
    event_timers_max_heap_size *= 2;
    if (event_timers_max_heap_size > EVENT_TIMERS_HEAP_INDEX_MASK) {
      php_critical_error ("maximum number of event timers exceeded");
    }
  }
  i = event_timer_heap_move_up(et->wakeup_time, i);
  et->heap_index = i | (wakeup_callback_id << (31 - MAX_WAKEUP_CALLBACKS_EXP));
  event_timers_heap[i] = et;
  return et;
}

void remove_event_timer(kphp_event_timer *et) {
  int i = (et->heap_index & EVENT_TIMERS_HEAP_INDEX_MASK);
  php_assert (i > 0 && i <= event_timers_heap_size && event_timers_heap[i] == et);
  et->heap_index = 0;//TODO remove after testing
  dl::deallocate(et, sizeof(kphp_event_timer));

  et = event_timers_heap[event_timers_heap_size--];
  if (i > event_timers_heap_size) {
    return;
  }

  i = event_timer_heap_move_up(et->wakeup_time, i);
  i = event_timer_heap_move_down(et->wakeup_time, i);
  et->heap_index &= ~EVENT_TIMERS_HEAP_INDEX_MASK;
  et->heap_index |= i;
  event_timers_heap[i] = et;
}

int remove_expired_event_timers() {
  int expired_events = 0;
  while (event_timers_heap_size > 0 && event_timers_heap[1]->wakeup_time <= get_precise_now()) {
    kphp_event_timer *et = event_timers_heap[1];
    wakeup_callbacks[et->heap_index >> (31 - MAX_WAKEUP_CALLBACKS_EXP)](et);
    expired_events++;
  }
  return expired_events;
}


int wait_net(int timeout_ms) {
  bool some_expires = false;//TODO remove assert
  double begin_time = get_precise_now();
  double expire_event_time = 0.0;
//  fprintf (stderr, "wait_net_begin\n");
  int finished_events = process_net_events();
  if (finished_events) {
    timeout_ms = 0;
  } else {
    if (event_timers_heap_size > 0 && event_timers_heap[1]->wakeup_time <= get_precise_now() + timeout_ms * 0.001) {
      if (timeout_ms > 0) {
        timeout_ms = timeout_convert_to_ms(event_timers_heap[1]->wakeup_time - get_precise_now());
      }
      some_expires = true;
      expire_event_time = event_timers_heap[1]->wakeup_time;
    }
  }

//  fprintf (stderr, "wait_net_middle %d\n", finished_events);
  wait_net_events(timeout_ms);
  finished_events += process_net_events();

  update_precise_now();
  finished_events += remove_expired_event_timers();

  if (some_expires && !finished_events) {
    php_warning("Have no finished events, but we must have them. begin_time = %.9lf, expire_event_time = %.9lf, timeout_ms = %d, now = %.9lf", begin_time, expire_event_time, timeout_ms, get_precise_now());
  }

//  fprintf (stderr, "wait_net_end %d\n", finished_events);

  return finished_events;
}

void init_net_events_lib() {
  event_timers_heap_size = 0;
  event_timers_max_heap_size = 1023;
  event_timers_heap = static_cast <kphp_event_timer **> (dl::allocate(sizeof(kphp_event_timer *) * event_timers_max_heap_size));

  update_precise_now();
}
