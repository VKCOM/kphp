// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/time-slice.h"

#include <unistd.h>

#include <gtest/gtest.h>

TEST(net_time_slice, basic) {
  using namespace vk::net;

  {
    const double expiration = 0.0001;
    TimeSlice time_slice(expiration);
    usleep(static_cast<unsigned int>(100 * (expiration * 1E6)));
    EXPECT_TRUE(time_slice.expired());
  }

  {
    const double expiration = 1;
    TimeSlice time_slice(expiration);
    EXPECT_FALSE(time_slice.expired());
  }
}
