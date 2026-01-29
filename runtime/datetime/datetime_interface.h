// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp/timelib/timelib.h"

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime/datetime/timelib_wrapper.h"

struct C$DateTimeInterface : public refcountable_polymorphic_php_classes_virt<> {
  timelib_time* time{nullptr};

  virtual const char* get_class() const noexcept = 0;
  virtual int get_hash() const noexcept = 0;
};

// Common helper for subclasses of C$DateTimeInterface
// NB: should be called under script allocator
inline timelib_time_offset* create_time_offset(timelib_time* t, [[maybe_unused]] const ScriptMemGuard& guard) {
  if (t->zone_type == TIMELIB_ZONETYPE_ABBR) {
    timelib_time_offset* offset = timelib_time_offset_ctor();
    offset->offset = (t->z + (t->dst * 3600));
    offset->leap_secs = 0;
    offset->is_dst = t->dst;
    offset->abbr = timelib_strdup(t->tz_abbr);
    return offset;
  }
  if (t->zone_type == TIMELIB_ZONETYPE_OFFSET) {
    timelib_time_offset* offset = timelib_time_offset_ctor();
    offset->offset = (t->z);
    offset->leap_secs = 0;
    offset->is_dst = 0;
    offset->abbr = static_cast<char*>(timelib_malloc(9)); // GMT±xxxx\0
    // set upper bound to 99 just to ensure that 'hours_offset' fits in %02d
    auto hours_offset = std::min(abs(offset->offset / 3600), 99);
    snprintf(offset->abbr, 9, "GMT%c%02d%02d", (offset->offset < 0) ? '-' : '+', hours_offset, abs((offset->offset % 3600) / 60));
    return offset;
  }
  return timelib_get_time_zone_info(t->sse, t->tz_info);
}
