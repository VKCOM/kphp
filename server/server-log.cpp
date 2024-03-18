// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <cstdarg>

#include "common/kprintf.h"

#include "server/json-logger.h"

#include "server/server-log.h"

const char *level2str(ServerLog type) {
  switch (type) {
    case ServerLog::Critical:
      return "Critical";
    case ServerLog::Error:
      return "Error";
    case ServerLog::Warning:
      return "Warning";
  }
  assert(0);
}

void write_log_server_impl(ServerLog type, const char *format, ...) noexcept {
  std::array<char, 2048> buffer{};

  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer.data(), buffer.size(), format, ap);
  va_end(ap);

  kprintf("%s: %s\n", level2str(type), buffer.data());
  vk::singleton<JsonLogger>::get().write_log_with_backtrace(buffer.data(), static_cast<int>(type));
}

void write_log_server_impl(ServerLog type, vk::string_view msg) noexcept {
  kprintf("%s: %.*s\n", level2str(type), static_cast<int>(msg.size()), msg.data());
  vk::singleton<JsonLogger>::get().write_log_with_backtrace(msg, static_cast<int>(type));
}
