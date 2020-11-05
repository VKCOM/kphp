// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_TIME_SLICE_H
#define KDB_NET_TIME_SLICE_H

#include "common/precise-time.h"

namespace vk {
namespace net {

class TimeSlice {
public:
  TimeSlice(double time_slice)
    : start_time_(get_network_time())
    , time_slice_(time_slice) {}

  bool expired() const {
    return start_time_ + time_slice_ < get_network_time();
  }

private:
  const double start_time_;
  const double time_slice_;
};

} // namespace net
} // namespace vk

#endif // KDB_NET_TIME_SLICE_H
