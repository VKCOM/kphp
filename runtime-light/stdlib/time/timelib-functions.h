// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

namespace kphp::timelib {

constexpr inline std::array<std::string_view, 12> MON_FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                                    "July",    "August",   "September", "October", "November", "December"};
constexpr inline std::array<std::string_view, 12> MON_SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr inline std::array<std::string_view, 7> DAY_FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr inline std::array<std::string_view, 7> DAY_SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

timelib_tzinfo* get_timezone_info() noexcept;

/**
 * @brief Retrieves a pointer to a `timelib_tzinfo` structure for a given time zone name.
 *
 * @param timezone The name of the time zone to retrieve.
 * @param tzdb The time zone database to search.
 * @param errc The pointer to a variable to store error code into.
 * @return `timelib_tzinfo*` pointing to the time zone information, or `nullptr` if not found.
 *
 * @note
 * - The returned pointer is owned by an internal cache; do not deallocate it using `timelib_tzinfo_dtor`.
 * - This function minimizes overhead by avoiding repeated allocations for the same time zone.
 */
timelib_tzinfo* get_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept;

std::tuple<std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::string_view, std::string_view>
getdate(std::int64_t timestamp, timelib_tzinfo& tzinfo) noexcept;

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept;

std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept;

std::optional<int64_t> strtotime(timelib_tzinfo& tzinfo, std::string_view datetime, int64_t timestamp) noexcept;

bool valid_date(int64_t year, int64_t month, int64_t day) noexcept;

} // namespace kphp::timelib
