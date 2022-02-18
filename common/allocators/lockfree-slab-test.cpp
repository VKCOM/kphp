// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/allocators/lockfree-slab.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

TEST(lockfree_slab, basic) {
  const std::size_t sizes[] = { 8, 12, 16, 31, 100 };

  for (const auto *size = std::begin(sizes); size != std::end(sizes); ++size) {
    const std::size_t pointers_size = 1000;

    std::vector<void*> pointers;
    pointers.reserve(pointers_size);

    lockfree_slab_cache_t cache;
    static __thread lockfree_slab_cache_tls_t cache_tls;
    lockfree_slab_cache_init(&cache, *size);
    lockfree_slab_cache_register_thread(&cache, &cache_tls);

    for (std::size_t i = 0; i < pointers_size; ++i) {
      pointers.push_back(lockfree_slab_cache_alloc(&cache_tls));
    }

    std::shuffle(pointers.begin(), pointers.end(), std::mt19937{});
    for(auto *pointer : pointers) {
      lockfree_slab_cache_free(&cache_tls, pointer);
    }

    lockfree_slab_cache_clear(&cache_tls);
  }
}

TEST(lockfree_slab, alloc0) {
  const std::size_t object_size = 32;
  lockfree_slab_cache_t cache;
  static __thread lockfree_slab_cache_tls_t cache_tls;
  lockfree_slab_cache_init(&cache, object_size);
  lockfree_slab_cache_register_thread(&cache, &cache_tls);

  const std::size_t pointers_size = 1000;
  std::vector<void*> pointers;
  pointers.reserve(pointers_size);
  for (std::size_t i = 0; i < pointers_size; ++i) {
    pointers.push_back(lockfree_slab_cache_alloc0(&cache_tls));
  }

  for(auto *pointer : pointers) {
    const auto *begin = static_cast<const char*>(pointer);
    const auto *end = std::next(begin, object_size);
    const auto *found = std::find_if(begin, end,[](const char value) { return value != 0; });

    EXPECT_EQ(found, end);
  }

  lockfree_slab_cache_clear(&cache_tls);
}

TEST(lockfree_slab, stress) {
  class locked_queue_t {
  public:
    void push(void *elem) {
      std::lock_guard<std::mutex> guard(mtx_);

      queue_.push(elem);
    }

    void* pop() {
      std::lock_guard<std::mutex> guard(mtx_);

      if (queue_.empty()) {
        return nullptr;
      }

      auto *elem = queue_.front();
      queue_.pop();

      return elem;
    }

  private:
    std::mutex mtx_;
    std::queue<void*> queue_;
  };

  lockfree_slab_cache_t cache;
  static __thread lockfree_slab_cache_tls_t cache_tls;
  lockfree_slab_cache_init(&cache, 8);

  constexpr std::size_t n_threads = 4;
  std::array<std::thread, n_threads> threads;
  std::array<std::array<locked_queue_t, n_threads>, n_threads> queues;

  std::atomic<std::size_t> finished_threads(0);

  for (std::size_t i = 0; i < n_threads; ++i) {
    auto work = [&](std::size_t thread_num) {
      lockfree_slab_cache_register_thread(&cache, &cache_tls);

      std::random_device rd;
      std::mt19937 g(rd());
      std::uniform_int_distribution<std::size_t> thread_num_distribution(0, n_threads - 1);

      for (std::size_t j = 0; j < 1000ULL; ++j) {
        std::vector<void *> in_buffers;
        auto &queue = queues[thread_num];
        for (auto &q : queue) {
          void *in_buffer = q.pop();
          while (in_buffer) {
            in_buffers.push_back(in_buffer);
            in_buffer = q.pop();
          }
        }
        std::shuffle(in_buffers.begin(), in_buffers.end(), g);

        constexpr std::size_t max_out_buffers_num = 1000;
        std::uniform_int_distribution<std::size_t> out_buffers_num_distribution(0, max_out_buffers_num);
        std::size_t out_buffers_num = out_buffers_num_distribution(g);
        std::vector<void *> out_buffers(out_buffers_num);

        while (out_buffers_num) {
          const auto out_thread_num = thread_num_distribution(g);
          std::uniform_int_distribution<std::size_t> out_buffers_part_distribution(0, out_buffers_num);
          const auto out_buffers_num_part = out_buffers_part_distribution(g);

          for (std::size_t i = 0; i < out_buffers_num_part; ++i) {
            void *out_buffer = lockfree_slab_cache_alloc(&cache_tls);
            queues[out_thread_num][thread_num].push(out_buffer);
          }

          out_buffers_num -= out_buffers_num_part;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(out_buffers_num));

        for (auto *in_buffer : in_buffers) {
          lockfree_slab_cache_free(&cache_tls, in_buffer);
        }
      }

      ++finished_threads;

      while (finished_threads != n_threads) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    };

    threads[i] = std::thread(work, i);
  }

  for (auto &thread : threads) {
    thread.join();
  }
}
