// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext-errors.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "vkext/vkext.h"

#define ERROR_BUFFER_SIZE 16384
static char vkext_error_buffer[ERROR_BUFFER_SIZE + 1];
static int error_codes[ERROR_BUFFER_SIZE + 1];
static char *error_start_ptr[ERROR_BUFFER_SIZE + 1];
static int errors_cnt;
static int vkext_error_buffer_ptr;

void vkext_reset_error() {
  vkext_error_buffer_ptr = 0;
  errors_cnt = 0;
}

void vkext_error(int error_code, const char *s) {
  if (errors_cnt < ERROR_BUFFER_SIZE) {
    error_codes[errors_cnt] = error_code;
    error_start_ptr[errors_cnt] = vkext_error_buffer + vkext_error_buffer_ptr;
    errors_cnt++;
  }
  while (vkext_error_buffer_ptr < ERROR_BUFFER_SIZE && *s) {
    vkext_error_buffer[vkext_error_buffer_ptr++] = *(s++);
  }
}

void vkext_error_format(int error_code, const char *format, ...) {
  assert(format);
  static char s[ERROR_BUFFER_SIZE + 1];
  va_list l;
  va_start(l, format);
  vsnprintf(s, ERROR_BUFFER_SIZE - vkext_error_buffer_ptr, format, l);
  vkext_error(error_code, s);
  va_end(l);
}

void vkext_get_errors(zval *result) {
  int i;
  error_start_ptr[errors_cnt] = vkext_error_buffer + vkext_error_buffer_ptr;
  array_init(result);
  for (i = 0; i < errors_cnt; i++) {
    zval *line;
    VK_ALLOC_INIT_ZVAL(line);
    array_init(line);
    add_next_index_long(line, error_codes[i]);
    vk_add_next_index_stringl(line, error_start_ptr[i], error_start_ptr[i + 1] - error_start_ptr[i]);
    add_next_index_zval(result, line);
  }
}
