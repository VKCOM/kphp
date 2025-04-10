// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/parallel/maximum.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#if !defined(__APPLE__)

TEST(parallel_maximum, basic) {
  for (int i = 0; i < 1000; ++i) {
    constexpr std::size_t thread_max = 1 * 1024 * 1024;

    PARALLEL_MAXIMUM(maximum);
    PARALLEL_MAXIMUM_INIT(maximum, thread_max);

    const std::size_t expected_max = 2LL * 1024 * 1024 * 1024;

    constexpr int nr_threads = 8;
    std::mt19937 random_engine(std::random_device{}());
    std::array<std::size_t, nr_threads> thread_consumption;

    std::size_t remains = expected_max;
    for (auto* t = thread_consumption.begin(); t < std::prev(thread_consumption.end()); ++t) {
      std::uniform_int_distribution<size_t> distribution(0, remains);
      *t = distribution(random_engine);
      assert(remains >= *t);
      remains -= *t;
    }
    thread_consumption.back() = remains;
    EXPECT_EQ(expected_max, std::accumulate(thread_consumption.begin(), thread_consumption.end(), 0LL));

    std::array<std::thread, nr_threads> threads;
    for (auto* t = threads.begin(); t < threads.end(); ++t) {
      const std::size_t consumption = thread_consumption[std::distance(threads.begin(), t)];

      *t = std::thread([=]() {
        parallel_register_thread();
        PARALLEL_MAXIMUM_REGISTER_THREAD(maximum);

        std::mt19937 random_engine(std::random_device{}());
        std::size_t consumed = 0;
        do {
          std::uniform_int_distribution<size_t> distribution(0, std::min(consumption - consumed, thread_max));
          const std::size_t to_consume = distribution(random_engine);
          PARALLEL_MAXIMUM_ADD(maximum, to_consume);
          consumed += to_consume;

        } while (consumed < consumption);
        EXPECT_EQ(consumed, consumption);

        PARALLEL_MAXIMUM_UNREGISTER_THREAD(maximum);
        parallel_unregister_thread();
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    EXPECT_NEAR(expected_max, PARALLEL_MAXIMUM_READ(maximum), nr_threads * thread_max + 1);
  }
}

#endif

TEST(parallel_maximum, add_in_one_thread_sub_in_another) {
  constexpr std::size_t thread_max = 1 * 1024 * 1024;

  PARALLEL_MAXIMUM(maximum);
  PARALLEL_MAXIMUM_INIT(maximum, thread_max);

  const std::size_t sizes[] = {48, 512, 2048, 16384, 262144};
  constexpr std::size_t expected_max = 128 * 1024 * 1024;

  std::mt19937 random_engine(std::random_device{}());
  std::uniform_int_distribution<int> distribution(0, std::extent<decltype(sizes), 0>::value - 1);

  auto producer = std::thread([&]() {
    parallel_register_thread();
    PARALLEL_MAXIMUM_REGISTER_THREAD(maximum);

    std::size_t produced = 0;
    while (produced < expected_max) {
      const std::size_t size = *std::next(std::begin(sizes), distribution(random_engine));
      PARALLEL_MAXIMUM_ADD(maximum, size);
      produced += size;
    }

    EXPECT_GE(produced, expected_max);

    PARALLEL_MAXIMUM_UNREGISTER_THREAD(maximum);
    parallel_unregister_thread();
  });
  producer.join();

  EXPECT_NEAR(PARALLEL_MAXIMUM_READ(maximum), expected_max, thread_max);

  auto consumer = std::thread([&]() {
    parallel_register_thread();
    PARALLEL_MAXIMUM_REGISTER_THREAD(maximum);

    for (;;) {
      const std::size_t size = *std::next(std::begin(sizes), distribution(random_engine));
      if (PARALLEL_MAXIMUM_READ_CURRENT(maximum) < size) {
        break;
      }
      PARALLEL_MAXIMUM_SUB(maximum, size);
    }

    PARALLEL_MAXIMUM_UNREGISTER_THREAD(maximum);
    parallel_unregister_thread();
  });
  consumer.join();

  EXPECT_NEAR(PARALLEL_MAXIMUM_READ_CURRENT(maximum), 0, thread_max);
  EXPECT_NEAR(PARALLEL_MAXIMUM_READ(maximum), expected_max, thread_max);
}
