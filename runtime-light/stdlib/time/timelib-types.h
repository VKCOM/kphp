// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <type_traits>

#include "kphp/timelib/timelib.h"

namespace kphp::timelib {

namespace details {

inline constexpr auto error_container_destructor{[](timelib_error_container* ec) noexcept { timelib_error_container_dtor(ec); }};
inline constexpr auto rel_time_destructor{[](timelib_rel_time* rt) noexcept { timelib_rel_time_dtor(rt); }};
inline constexpr auto time_offset_destructor{[](timelib_time_offset* to) noexcept { timelib_time_offset_dtor(to); }};
inline constexpr auto time_destructor{[](timelib_time* t) noexcept { timelib_time_dtor(t); }};
inline constexpr auto tzinfo_destructor{[](timelib_tzinfo* t) noexcept { timelib_tzinfo_dtor(t); }};

} // namespace details

using error_container_holder = std::unique_ptr<timelib_error_container, std::remove_cvref_t<decltype(kphp::timelib::details::error_container_destructor)>>;
using rel_time_holder = std::unique_ptr<timelib_rel_time, std::remove_cvref_t<decltype(kphp::timelib::details::rel_time_destructor)>>;
using time_offset_holder = std::unique_ptr<timelib_time_offset, std::remove_cvref_t<decltype(kphp::timelib::details::time_offset_destructor)>>;
using time_holder = std::unique_ptr<timelib_time, std::remove_cvref_t<decltype(kphp::timelib::details::time_destructor)>>;
using tzinfo_holder = std::unique_ptr<timelib_tzinfo, std::remove_cvref_t<decltype(kphp::timelib::details::tzinfo_destructor)>>;

} // namespace kphp::timelib
