// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/stats/buffer.h"

#include <cstdio>

void sb_init(stats_buffer_t *sb, char *buff, int size) {
  sb->buff = buff;
  sb->pos = 0;
  sb->size = size;
  sb->overflowed = 0;
}

static void sb_truncate(stats_buffer_t *sb) {
  sb->overflowed = 1;
  sb->buff[sb->size - 1] = 0;
  sb->pos = sb->size - 2;
  while (sb->pos >= 0 && sb->buff[sb->pos] != '\n') {
    sb->buff[sb->pos--] = 0;
  }
  sb->pos++;
}

void sb_printf(stats_buffer_t *sb, const char *format, ...) {
  if (sb->overflowed) {
    return;
  }
  va_list ap;
  va_start(ap, format);
  sb->pos += vsnprintf(sb->buff + sb->pos, sb->size - sb->pos, format, ap);
  va_end(ap);
  if (sb->pos >= sb->size) {
    sb_truncate(sb);
  }
}

void vsb_printf(stats_buffer_t *sb, const char *format, va_list __arg_list) {
  if (sb->overflowed) {
    return;
  }
  sb->pos += vsnprintf(sb->buff + sb->pos, sb->size - sb->pos, format, __arg_list);
  if (sb->pos >= sb->size) {
    sb_truncate(sb);
  }
}

void sb_append(stats_buffer_t *sb, char c) {
  if (sb->overflowed) {
    return;
  }
  sb->buff[sb->pos++] = c;
  if (sb->pos >= sb->size) {
    sb_truncate(sb);
  }
  sb->buff[sb->pos] = 0;
}
