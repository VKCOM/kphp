// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <type_traits>

#include "kphp/timelib/timelib.h"

#include "runtime-light/allocator/allocator.h"

namespace kphp::timelib {

namespace details {

inline constexpr auto error_container_destructor{
    [](timelib_error_container* ec) noexcept { kphp::memory::libc_alloc_guard{}, timelib_error_container_dtor(ec); }};
inline constexpr auto rel_time_destructor{[](timelib_rel_time* rt) noexcept { kphp::memory::libc_alloc_guard{}, timelib_rel_time_dtor(rt); }};
inline constexpr auto time_offset_destructor{[](timelib_time_offset* to) noexcept { kphp::memory::libc_alloc_guard{}, timelib_time_offset_dtor(to); }};
inline constexpr auto time_destructor{[](timelib_time* t) noexcept { kphp::memory::libc_alloc_guard{}, timelib_time_dtor(t); }};
inline constexpr auto tzinfo_destructor{[](timelib_tzinfo* t) noexcept { kphp::memory::libc_alloc_guard{}, timelib_tzinfo_dtor(t); }};

} // namespace details

using error_container = std::unique_ptr<timelib_error_container, std::remove_cvref_t<decltype(kphp::timelib::details::error_container_destructor)>>;
using rel_time = std::unique_ptr<timelib_rel_time, std::remove_cvref_t<decltype(kphp::timelib::details::rel_time_destructor)>>;
using time_offset = std::unique_ptr<timelib_time_offset, std::remove_cvref_t<decltype(kphp::timelib::details::time_offset_destructor)>>;
using time = std::unique_ptr<timelib_time, std::remove_cvref_t<decltype(kphp::timelib::details::time_destructor)>>;
using tzinfo = std::unique_ptr<timelib_tzinfo, std::remove_cvref_t<decltype(kphp::timelib::details::tzinfo_destructor)>>;

} // namespace kphp::timelib
