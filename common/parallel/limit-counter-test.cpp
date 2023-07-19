// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>
#include <random>
#include <thread>

#include "common/parallel/limit-counter.h"

#if !defined(__APPLE__)

TEST(parallel_limit_counter, basic) {
  constexpr std::size_t global_max = 100000;
  constexpr std::size_t thread_max = 10000;

  PARALLEL_LIMIT_COUNTER(limit_counter);
  PARALLEL_LIMIT_COUNTER_INIT(limit_counter, global_max, thread_max);

  constexpr int nr_threads = 8;
  std::mt19937 random_engine(std::random_device{}());
  std::uniform_int_distribution<int> distribution(0, thread_max);

  std::array<std::thread, nr_threads> threads;

  std::uint64_t expected_sum = 0;
  for (auto &thread : threads) {
    const int counter = distribution(random_engine);
    expected_sum += counter;

    thread = std::thread([=]() {
      parallel_register_thread();
      PARALLEL_LIMIT_COUNTER_REGISTER_THREAD(limit_counter);

      for (std::uint64_t i = 0; i < counter; ++i) {
        PARALLEL_LIMIT_COUNTER_INC(limit_counter);
      }

      PARALLEL_LIMIT_COUNTER_UNREGISTER_THREAD(limit_counter);
      parallel_unregister_thread();
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(expected_sum, PARALLEL_LIMIT_COUNTER_READ(limit_counter));
}

TEST(parallel_limit_counter, accuracy) {
  constexpr std::size_t global_max = 100000000;
  constexpr std::size_t thread_max = 1000;

  PARALLEL_LIMIT_COUNTER(limit_counter);
  PARALLEL_LIMIT_COUNTER_INIT(limit_counter, global_max, thread_max);

  constexpr int nr_threads = 8;
  std::array<std::thread, nr_threads> threads;
  std::array<std::atomic<std::size_t>, nr_threads> thread_counters;
  std::fill(thread_counters.begin(), thread_counters.end(), UINT64_C(0));

  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i] = std::thread([&thread_counters, i] {
      std::mt19937 random_engine(std::random_device{}());
      std::uniform_int_distribution<int> distribution(0, thread_max);

      parallel_register_thread();
      PARALLEL_LIMIT_COUNTER_REGISTER_THREAD(limit_counter);

      for (;;) {
        const std::size_t count = distribution(random_engine);
        if (!PARALLEL_LIMIT_COUNTER_ADD(limit_counter, count)) {
          break;
        }
        thread_counters[i] += count;
      }

      PARALLEL_LIMIT_COUNTER_UNREGISTER_THREAD(limit_counter);
      parallel_unregister_thread();
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  const std::size_t sum = std::accumulate(thread_counters.begin(), thread_counters.end(), UINT64_C(0));
  EXPECT_NEAR(sum, PARALLEL_LIMIT_COUNTER_READ(limit_counter), nr_threads * thread_max);
}

#endif
