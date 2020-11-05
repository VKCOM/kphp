// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
  char *buff;
  int pos;
  int size;
  int overflowed;
} stats_buffer_t;

void sb_init(stats_buffer_t *sb, char *buff, int size);
void sb_printf(stats_buffer_t *sb, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void vsb_printf(stats_buffer_t *sb, const char *format, va_list __arg_list);
void sb_append(stats_buffer_t *sb, char c);
