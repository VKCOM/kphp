// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

namespace kphp::timelib {

namespace timezones {

inline constexpr std::string_view MOSCOW = "Europe/Moscow";
inline constexpr std::string_view GMT3 = "Etc/GMT-3";
inline constexpr std::string_view GMT4 = "Etc/GMT-4";

} // namespace timezones

} // namespace kphp::timelib
