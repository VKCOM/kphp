#include <gtest/gtest.h>
#include <mutex>
#include <sys/syscall.h>
#include <thread>

#include "runtime/inter-process-mutex.h"
#include "server/php-engine-vars.h"

TEST(inter_process_mutex_test, test_lock_unlock) {
  inter_process_mutex mutex;
  mutex.lock();
  mutex.unlock();
}

TEST(inter_process_mutex_test, test_try_lock_unlock) {
  inter_process_mutex mutex;
  ASSERT_TRUE(mutex.try_lock());
  mutex.unlock();
}

namespace {

template<class F>
auto with_this_pid(F f) noexcept {
  static std::mutex pid_mutex;
  std::lock_guard<std::mutex> l{pid_mutex};
  pid = syscall(SYS_gettid);
  return f();
}

} // namespace

TEST(inter_process_mutex_test, test_lock_unlock_with_thread) {
  inter_process_mutex mutex;

  size_t active_threads = 0;
  std::vector<std::thread> threads;
  for (size_t i = 0; i < 10; ++i) {
    threads.emplace_back(std::thread{[&mutex, &active_threads] {
      with_this_pid([&mutex] { mutex.lock(); });
      ASSERT_EQ(++active_threads, 1u);
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
      ASSERT_EQ(--active_threads, 0u);
      with_this_pid([&mutex] { mutex.unlock(); });
    }});
  }

  for (auto &t : threads) {
    t.join();
  }
  ASSERT_EQ(active_threads, 0u);
}

TEST(inter_process_mutex_test, test_lock_unlock_with_thread_check_waiting) {
  inter_process_mutex mutex;
  size_t active_threads = 0;

  std::thread t1{[&mutex, &active_threads] {
    with_this_pid([&mutex] { mutex.lock(); });
    ASSERT_EQ(++active_threads, 1u);

    std::thread t2{[&mutex, &active_threads] {
      with_this_pid([&mutex] { return mutex.lock(); });
      ASSERT_EQ(++active_threads, 1u);
      std::this_thread::sleep_for(std::chrono::milliseconds{54});
      ASSERT_EQ(--active_threads, 0u);
      with_this_pid([&mutex] { mutex.unlock(); });
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds{123});
    ASSERT_EQ(--active_threads, 0u);
    with_this_pid([&mutex] { mutex.unlock(); });

    t2.join();
    ASSERT_EQ(active_threads, 0u);
  }};

  t1.join();
  ASSERT_EQ(active_threads, 0u);
}

TEST(inter_process_mutex_test, test_try_lock_unlock_with_thread) {
  inter_process_mutex mutex;

  std::thread t{[&mutex] {
    ASSERT_TRUE(with_this_pid([&mutex] { return mutex.try_lock(); }));

    std::thread{[&mutex] { ASSERT_FALSE(with_this_pid([&mutex] { return mutex.try_lock(); })); }}.join();

    with_this_pid([&mutex] { mutex.unlock(); });
  }};

  t.join();
}
