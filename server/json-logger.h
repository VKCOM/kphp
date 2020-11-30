// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <array>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

class JsonLogger : vk::not_copyable {
public:
  static JsonLogger &get() noexcept;

  bool tags_are_set() const noexcept {
    return !tags_.empty();
  }

  bool extra_info_is_set() const noexcept {
    return !extra_info_.empty();
  }

  void set_tags(vk::string_view tags) noexcept;
  void set_extra_info(vk::string_view extra_info) noexcept;
  void set_env(vk::string_view env) noexcept;
  void reset() noexcept;

  vk::string_view get_tags() const noexcept {
    return tags_;
  }

  vk::string_view get_extra_info() const noexcept {
    return extra_info_;
  }

  vk::string_view get_env() const noexcept {
    return env_;
  }

private:
  JsonLogger() = default;

  vk::string_view tags_;
  vk::string_view extra_info_;
  vk::string_view env_;

  std::array<char, 10 * 1024> tags_buffer_{0};
  std::array<char, 10 * 1024> extra_info_buffer_{0};
  std::array<char, 128> env_buffer_{0};
};
