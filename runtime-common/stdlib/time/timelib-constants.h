// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>

namespace kphp::timelib {

namespace timezones {

inline constexpr const char* MOSCOW = "Europe/Moscow";
inline constexpr const char* GMT3 = "Etc/GMT-3";
inline constexpr const char* GMT4 = "Etc/GMT-4";

} // namespace timezones

namespace months {

constexpr inline std::array<const char*, 12> FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                           "July",    "August",   "September", "October", "November", "December"};
constexpr inline std::array<const char*, 12> SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

} // namespace months

namespace days {

constexpr inline std::array<const char*, 7> FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr inline std::array<const char*, 7> SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

} // namespace days

} // namespace kphp::timelib
