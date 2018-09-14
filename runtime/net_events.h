#pragma once

const int MAX_TIMEOUT = 86400;
const int MAX_TIMEOUT_MS = MAX_TIMEOUT * 1000;

int timeout_convert_to_ms(double timeout);


void update_precise_now();

double get_precise_now();


struct event_timer {
  int heap_index;
  int wakeup_extra;
  double wakeup_time;
};

int register_wakeup_callback(void (*wakeup)(int wakeup_extra));

event_timer *allocate_event_timer(double wakeup_time, int wakeup_callback_id, int wakeup_extra);

void remove_event_timer(event_timer *et);


int wait_net(int timeout_ms);


void net_events_init_static();
