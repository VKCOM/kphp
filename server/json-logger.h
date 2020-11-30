// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <array>

#include "common/mixin/not_copyable.h"

class JsonLogger: vk::not_copyable {
public:
  static JsonLogger& get();

  bool tags_are_set() const {
    return tags_buffer[0] != '\0';
  }

  bool extra_info_is_set() const {
    return extra_info_buffer[0] != '\0';
  }

  void set_extra_info(const char *ptr, size_t size);
  void set_tags(const char *ptr, size_t size);
  void set_env(const char *ptr, size_t size);
  void reset();

  const char *tags_c_str() const {
    return tags_buffer;
  }

  const char *extra_info_c_str() const {
    return extra_info_buffer;
  }

  const char *env_c_str() const {
    return env_buffer;
  }
private:
  JsonLogger() = default;
  char tags_buffer[10 * 1024]{0};
  char extra_info_buffer[10 * 1024]{0};
  char env_buffer[128]{0};
};
