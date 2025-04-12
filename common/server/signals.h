// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <signal.h>

void kill_main();

typedef void (*extra_debug_handler_t)();
extern extra_debug_handler_t extra_debug_handler;
extern int daemonize;

void print_backtrace();
void ksignal(int sig, void (*handler)(int));
void ksignal_intr(int sig, void (*handler)(int));
void set_debug_handlers(bool save_signal_in_exit_code = false);
void setup_delayed_handlers();
int is_signal_pending(int sig);

const char *signal_shortname(int sig);

extern volatile long long pending_signals;

static inline void empty_handler (int sig __attribute__((unused))) { }
