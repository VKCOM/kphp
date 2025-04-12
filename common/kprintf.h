// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KPRINTF_H__
#define __KPRINTF_H__

#include <stdio.h>

#include "common/pid.h"

extern int verbosity;
extern const char *logname;

extern long long log_not_too_much_total;

void reopen_logs ();
void reopen_logs_ext (int slave_mode);
int hexdump(FILE* file, const void *start, const void *end);

// write message with timestamp and pid, safe to call inside handler
int kwrite (int fd, const void *buf, int count);

void kprintf_multiprocessing_mode_enable ();
#ifdef __CLION_IDE__
void kprintf (const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void vkprintf (int verbosity, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
#else
// print message with timestamp
void kprintf_ (const char *file, int line, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
#define kprintf(...) kprintf_(__FILE__, __LINE__, __VA_ARGS__)
#define vkprintf(verbosity_level, format, ...) do { \
    if ((verbosity_level) > verbosity) { \
      break; \
    } \
    kprintf ((format), ##__VA_ARGS__); \
  } while (0)
#endif

// If n errors occurred, log_2 (n) messages will be written
#define do_not_too_much(do_fun, ...)                          \
do {                                                          \
  static long long errors_counter = 0;                        \
  errors_counter++;                                           \
  log_not_too_much_total++;                                   \
  if ((errors_counter & (errors_counter - 1)) == 0) {         \
    do_fun(__VA_ARGS__);                                      \
  }                                                           \
} while (0)

#define log_not_too_much_print_impl(fun, ...)           \
fun("ERROR (total %lld occurred): ", errors_counter);   \
fprintf(stderr, __VA_ARGS__);

#define log_not_too_much_fun(fun, ...) do_not_too_much(log_not_too_much_print_impl, fun, __VA_ARGS__)

#define log_not_too_much(...)                               log_not_too_much_fun(kprintf, __VA_ARGS__)
#define send_not_too_much_messages_to_assertion_chat(...)   log_not_too_much_fun(send_message_to_assertion_chat, __VA_ARGS__)

#define tv_do(func, type, verbosity_level, format, ...) do { \
    if (!VERBOSITY(type, verbosity_level)) { \
      break; \
    } \
    func ((format), ##__VA_ARGS__); \
  } while (0)

#define tvkprintf(...) tv_do(kprintf, ##__VA_ARGS__)
#define tvlog_not_too_much(...) tv_do(log_not_too_much, ##__VA_ARGS__)


#define VERBOSITY(nm, level) ((verbosity_ ##nm == -1 ? verbosity : verbosity_ ##nm) >= (level))
#define DECLARE_VERBOSITY(nm) extern int verbosity_ ##nm

#define DEFINE_VERBOSITY(nm)                                                                                                \
int verbosity_ ##nm = -1;                                               \
static void register_verbosity_ ## nm() __attribute__((constructor));   \
static void register_verbosity_ ## nm() {                               \
  register_verbosity_type_inner(#nm, &verbosity_ ##nm);                 \
}

void send_message_to_assertion_chat_va_list(const char *message, va_list arg_ptr);
void send_message_to_assertion_chat(const char *message, ...) __attribute__ ((format (printf, 1, 2)));
void register_verbosity_type_inner(const char *name, int *verbosity);
int set_verbosity_by_type(const char *type_name, int value);


const char *ip_to_print(unsigned ip);
const char *ipv6_to_print(const void *ip);
const char *pid_to_print(const struct process_id *pid);


#endif
