// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

constexpr int MAX_TIMEOUT = 86400;
constexpr int MAX_TIMEOUT_MS = MAX_TIMEOUT * 1000;

int timeout_convert_to_ms(double timeout);

void update_precise_now();

double get_precise_now();

struct kphp_event_timer {
  int heap_index;
  int wakeup_extra;
  double wakeup_time;
};

int register_wakeup_callback(void (*wakeup)(kphp_event_timer *timer));

kphp_event_timer *allocate_event_timer(double wakeup_time, int wakeup_callback_id, int wakeup_extra);

void remove_event_timer(kphp_event_timer *et);

int wait_net(int timeout_ms);

void init_net_events_lib();
