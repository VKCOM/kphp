// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include <cstring>

#include "server/json-logger.h"

namespace {

template<size_t N>
void copy_if_enough_size(vk::string_view src, vk::string_view &dest, std::array<char, N> &buffer) noexcept {
  if (src.size() <= buffer.size()) {
    std::copy(src.begin(), src.end(), buffer.begin());
    dest = {buffer.data(), src.size()};
  }
}

} // namespace

JsonLogger &JsonLogger::get() noexcept {
  static JsonLogger logger;
  return logger;
}

void JsonLogger::set_tags(vk::string_view tags) noexcept {
  copy_if_enough_size(tags, tags_, tags_buffer_);
}

void JsonLogger::set_extra_info(vk::string_view extra_info) noexcept {
  copy_if_enough_size(extra_info, extra_info_, extra_info_buffer_);
}

void JsonLogger::set_env(vk::string_view env) noexcept {
  copy_if_enough_size(env, env_, env_buffer_);
}

void JsonLogger::reset() noexcept {
  tags_ = {};
  extra_info_ = {};
  env_ = {};
}
