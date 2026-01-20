// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp/timelib/timelib.h"

#include "runtime-light/allocator/allocator.h"

namespace kphp::timelib {

namespace details {

struct error_container_destructor {
  void operator()(timelib_error_container* ec) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_error_container_dtor(ec);
  }
};

struct rel_time_destructor {
  void operator()(timelib_rel_time* rt) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_rel_time_dtor(rt);
  }
};

struct time_offset_destructor {
  void operator()(timelib_time_offset* to) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_time_offset_dtor(to);
  }
};

struct time_destructor {
  void operator()(timelib_time* t) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_time_dtor(t);
  }
};

struct tzinfo_destructor {
  void operator()(timelib_tzinfo* t) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_tzinfo_dtor(t);
  }
};

} // namespace details

using error_container = std::unique_ptr<timelib_error_container, kphp::timelib::details::error_container_destructor>;
using rel_time = std::unique_ptr<timelib_rel_time, kphp::timelib::details::rel_time_destructor>;
using time_offset = std::unique_ptr<timelib_time_offset, kphp::timelib::details::time_offset_destructor>;
using time = std::unique_ptr<timelib_time, kphp::timelib::details::time_destructor>;
using tzinfo = std::unique_ptr<timelib_tzinfo, kphp::timelib::details::tzinfo_destructor>;

} // namespace kphp::timelib
