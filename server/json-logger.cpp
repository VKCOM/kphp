// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include <cstring>

#include "server/json-logger.h"


JsonLogger &JsonLogger::get() {
  static JsonLogger logger;
  return logger;
}

void JsonLogger::set_tags(const char *ptr, size_t size) {
  if ((size + 1) > sizeof(tags_buffer)) {
    return;
  }

  memcpy(tags_buffer, ptr, size);
  tags_buffer[size] = '\0';
}

void JsonLogger::set_extra_info(const char *ptr, size_t size) {
  if ((size + 1) > sizeof(extra_info_buffer)) {
    return;
  }

  memcpy(extra_info_buffer, ptr, size);
  extra_info_buffer[size] = '\0';
}

void JsonLogger::set_env(const char *ptr, size_t size) {
  if ((size + 1) > sizeof(env_buffer)) {
    return;
  }

  memcpy(env_buffer, ptr, size);
  env_buffer[size] = '\0';
}

void JsonLogger::reset() {
  extra_info_buffer[0] = '\0';
  tags_buffer[0] = '\0';
  env_buffer[0] = '\0';
}
