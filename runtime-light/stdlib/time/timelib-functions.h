// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

namespace kphp::timelib {

/**
 * @brief Retrieves a pointer to a `timelib_tzinfo` structure for a given time zone name.
 *
 * @param timezone The name of the time zone to retrieve.
 * @param tzdb The time zone database to search.
 * @param errc The address of a variable to store error code into.
 * @return `timelib_tzinfo*` pointing to the time zone information, or `nullptr` if not found.
 *
 * @note
 * - The returned pointer is owned by an internal cache; do not deallocate it using `timelib_tzinfo_dtor`.
 * - This function minimizes overhead by avoiding repeated allocations for the same time zone.
 */
timelib_tzinfo* get_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept;

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view datetime, int64_t timestamp) noexcept;

} // namespace kphp::timelib
