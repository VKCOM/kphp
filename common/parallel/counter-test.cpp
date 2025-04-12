// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/counter.h"

#include <cstdint>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#if !defined(__APPLE__)

TEST(parallel_counter, basic) {
  PARALLEL_COUNTER(counter);
  const int nr_threads = 8;
  auto random_engine = std::default_random_engine();
  std::uniform_int_distribution<int> distribution(0, 100000);

  std::vector<std::thread> threads(nr_threads);

  std::uint64_t expected_sum = 0;
  for (auto &thread : threads) {
    const int counter = distribution(random_engine);
    expected_sum += counter;

    thread = std::thread([=]() {
      parallel_register_thread();
      PARALLEL_COUNTER_REGISTER_THREAD(counter);
      for (std::uint64_t i = 0; i < counter; ++i) {
        PARALLEL_COUNTER_INC(counter);
      }
      PARALLEL_COUNTER_UNREGISTER_THREAD(counter);
      parallel_unregister_thread();
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(expected_sum, PARALLEL_COUNTER_READ(counter));
}

#endif

TEST(parallel_counter, inc_and_dec) {
  PARALLEL_COUNTER(counter);

  parallel_register_thread();
  PARALLEL_COUNTER_REGISTER_THREAD(counter);

  EXPECT_EQ(0, PARALLEL_COUNTER_READ(counter));
  PARALLEL_COUNTER_INC(counter);
  EXPECT_EQ(1, PARALLEL_COUNTER_READ(counter));
  PARALLEL_COUNTER_DEC(counter);
  EXPECT_EQ(0, PARALLEL_COUNTER_READ(counter));

  PARALLEL_COUNTER_UNREGISTER_THREAD(counter);
  parallel_unregister_thread();
}
