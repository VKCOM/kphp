// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "php_assert.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>


//const char *engine_tag = "[";
//const char *engine_pid = "] ";
//
//int php_disable_warnings = 0;
//int php_warning_level = 2;
//int php_warning_minimum_level = 0;

// linker magic: run_scheduler function is declared in separate section.
// their addresses could be used to check if address is inside run_scheduler
struct nothing {};
extern nothing __start_run_scheduler_section;
extern nothing __stop_run_scheduler_section;
static bool is_address_inside_run_scheduler(void *address) {
  return true;
};

static void print_demangled_adresses(void **buffer, int nptrs, int num_shift, bool allow_gdb) {

}

static void php_warning_impl(bool out_of_memory, int error_type, char const *message, va_list args) {

}

void php_notice(char const *message, ...) {

}

void php_warning(char const *message, ...) {

}

void php_error(char const *message, ...) {

}

void php_out_of_memory_warning(char const *message, ...) {

}


void php_assert__(const char *msg, const char *file, int line) {

}

void raise_php_assert_signal__() {

}
