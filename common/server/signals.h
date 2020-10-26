#pragma once

#include <signal.h>

void kill_main(void);

typedef void (*extra_debug_handler_t)(void);
extern extra_debug_handler_t extra_debug_handler;
extern int daemonize;

void print_backtrace(void);
void ksignal(int sig, void (*handler)(int));
void ksignal_intr(int sig, void (*handler)(int));
void set_debug_handlers(void);
void setup_delayed_handlers();
int is_signal_pending(int sig);
void set_signals_handlers (void);

const char *signal_shortname(int sig);

extern volatile long long pending_signals;

static inline void empty_handler (int sig __attribute__((unused))) { }
