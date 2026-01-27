// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

struct _timelib_error_container;

struct _timelib_time_offset;

struct _timelib_time;

struct _timelib_rel_time;

struct _timelib_tzinfo;

namespace kphp::timelib {

using error_container = _timelib_error_container;

using time_offset = _timelib_time_offset;

using time = _timelib_time;

using rel_time = _timelib_rel_time;

using tzinfo = _timelib_tzinfo;

} // namespace kphp::timelib
