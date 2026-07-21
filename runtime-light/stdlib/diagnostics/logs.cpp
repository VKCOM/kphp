// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/logs.h"

#include <array>
#include <cstddef>
#include <source_location>
#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

void kphp::log::assertion(bool condition, const std::source_location& location) noexcept {
  if (!condition) [[unlikely]] {
    std::array<char, impl::DEFAULT_LOG_BUFFER_SIZE> log_buffer; // NOLINT
    size_t message_size{
        impl::format_log_message(log_buffer, "assertion failed at {}:{}",
                                 std::make_format_args(kphp::log::impl::unmove(location.file_name()), kphp::log::impl::unmove(location.line())))};
    auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(message_size)}};
    k2::log(std::to_underlying(level::error), message, {});
    k2::exit(1);
  }
}
