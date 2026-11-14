#include <coroutine>
#include <cstdint>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"
#include "runtime-light/stdlib/diagnostics/pause-timer-awaitable.h"
#include "runtime/curl-network-wait.h"

namespace {

struct TestTimer {
  bool pause() noexcept {
    ++pause_calls;
    return std::exchange(running, false);
  }

  void resume() noexcept {
    if (!running) {
      ++resume_calls;
      running = true;
    }
  }

  bool running{true};
  int pause_calls{0};
  int resume_calls{0};
};

struct TestAwaiter {
  bool await_ready() const noexcept {
    return ready;
  }

  void await_suspend(std::coroutine_handle<>) noexcept {}

  int await_resume() const noexcept {
    return 42;
  }

  bool ready{false};
};

TEST(builtin_time_stats_test, nested_pause_guard_preserves_pause_owner) {
  uint64_t total{0};
  uint64_t method{0};
  BuiltinTimeGuard timer{total, method};

  {
    BuiltinTimePauseGuard outer{timer};
    { BuiltinTimePauseGuard inner{timer}; }
    EXPECT_FALSE(timer.pause());
  }

  EXPECT_TRUE(timer.pause());
}

TEST(builtin_time_stats_test, ready_awaiter_does_not_pause_timer) {
  TestTimer timer;
  PauseTimerAwaiter awaiter{TestAwaiter{.ready = true}, &timer};

  EXPECT_TRUE(awaiter.await_ready());
  EXPECT_EQ(awaiter.await_resume(), 42);
  EXPECT_TRUE(timer.running);
  EXPECT_EQ(timer.pause_calls, 0);
  EXPECT_EQ(timer.resume_calls, 0);
}

TEST(builtin_time_stats_test, suspended_awaiter_resumes_owned_pause) {
  TestTimer timer;
  PauseTimerAwaiter awaiter{TestAwaiter{}, &timer};

  EXPECT_FALSE(awaiter.await_ready());
  awaiter.await_suspend(std::noop_coroutine());
  EXPECT_FALSE(timer.running);
  EXPECT_EQ(awaiter.await_resume(), 42);
  EXPECT_TRUE(timer.running);
  EXPECT_EQ(timer.pause_calls, 1);
  EXPECT_EQ(timer.resume_calls, 1);
}

TEST(builtin_time_stats_test, nested_awaiter_does_not_resume_outer_pause) {
  TestTimer timer;
  PauseTimerAwaiter outer{TestAwaiter{}, &timer};
  PauseTimerAwaiter inner{TestAwaiter{}, &timer};

  outer.await_suspend(std::noop_coroutine());
  inner.await_suspend(std::noop_coroutine());
  EXPECT_FALSE(timer.running);

  EXPECT_EQ(inner.await_resume(), 42);
  EXPECT_FALSE(timer.running);
  EXPECT_EQ(timer.resume_calls, 0);

  EXPECT_EQ(outer.await_resume(), 42);
  EXPECT_TRUE(timer.running);
  EXPECT_EQ(timer.resume_calls, 1);
}

TEST(builtin_time_stats_test, destroying_suspended_awaiter_resumes_timer) {
  TestTimer timer;
  {
    PauseTimerAwaiter awaiter{TestAwaiter{}, &timer};
    awaiter.await_suspend(std::noop_coroutine());
    EXPECT_FALSE(timer.running);
  }

  EXPECT_TRUE(timer.running);
  EXPECT_EQ(timer.pause_calls, 1);
  EXPECT_EQ(timer.resume_calls, 1);
}

TEST(builtin_time_stats_test, counters_accumulate_and_reset_per_request) {
  enum class Method { first, second };
  BuiltinTimeStats<Method, 2> stats;
  {
    auto timer = stats.write(Method::first);
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  {
    auto timer = stats.write(Method::second);
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
  }
  EXPECT_GE(stats.methods[0], 1000);
  EXPECT_GE(stats.methods[1], 2000);
  EXPECT_EQ(stats.total, stats.methods[0] + stats.methods[1]);
  stats.reset();
  EXPECT_EQ(stats.total, 0);
  EXPECT_EQ(stats.methods[0], 0);
  EXPECT_EQ(stats.methods[1], 0);
}

TEST(builtin_time_stats_test, continuous_wall_includes_internal_waits_and_subtracts_only_network) {
  uint64_t total{}, method{};
  {
    BuiltinTimeGuard timer{total, method};
    std::this_thread::sleep_for(std::chrono::milliseconds{10}); // lock/IPC/queue
    const auto network_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    timer.subtract_elapsed_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - network_start).count());
    std::this_thread::sleep_for(std::chrono::milliseconds{10}); // parsing/callback
  }
  EXPECT_GE(total, 20'000'000);
  EXPECT_EQ(total, method);
}

TEST(builtin_time_stats_test, network_subtraction_does_not_underflow) {
  uint64_t total{}, method{};
  {
    BuiltinTimeGuard timer{total, method};
    timer.subtract_elapsed_ns(UINT64_MAX);
  }
  EXPECT_EQ(total, 0);
  EXPECT_EQ(method, 0);
}

TEST(builtin_time_stats_test, synchronous_network_attribution_is_local_and_restored) {
  uint64_t first_total{}, first_method{}, second_total{}, second_method{};
  {
    BuiltinTimeGuard first{first_total, first_method};
    BuiltinTimeGuard second{second_total, second_method};
    {
      BuiltinTimeNetworkScope outer{first};
      EXPECT_EQ(BuiltinTimeNetworkScope::current(), &first);
      {
        BuiltinTimeNetworkScope inner{second};
        EXPECT_EQ(BuiltinTimeNetworkScope::current(), &second);
        const auto started = BuiltinTimeClock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        BuiltinTimeNetworkScope::current()->subtract_elapsed_ns(
            std::chrono::duration_cast<std::chrono::nanoseconds>(BuiltinTimeClock::now() - started).count());
      }
      EXPECT_EQ(BuiltinTimeNetworkScope::current(), &first);
    }
    EXPECT_EQ(BuiltinTimeNetworkScope::current(), nullptr);
  }
  EXPECT_GE(first_total, 10'000'000); // another call's network must not alter this timer
  EXPECT_GE(first_total - second_total, 9'000'000);
}

TEST(builtin_time_stats_test, fatal_longjmp_attribution_can_be_reset) {
  uint64_t total{}, method{};
  BuiltinTimeGuard timer{total, method};
  BuiltinTimeNetworkScope scope{timer};
  BuiltinTimeNetworkScope::reset();
  EXPECT_EQ(BuiltinTimeNetworkScope::current(), nullptr);
}

TEST(builtin_time_stats_test, async_curl_counts_ready_queue_and_multiple_socket_dispatch) {
  CurlNetworkWait wait;
  wait.begin();
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  wait.ready(1);
  wait.ready(2);
  const auto network_ns = wait.elapsed_ns();
  EXPECT_GE(network_ns, 5'000'000);
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  wait.consume(1);
  wait.begin(); // socket 2 is already ready: no new network wait
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  wait.consume(2);
  EXPECT_EQ(wait.elapsed_ns(), network_ns);
  wait.begin();
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  wait.end();
  EXPECT_GE(wait.elapsed_ns(), network_ns + 5'000'000);
}

TEST(builtin_time_stats_test, async_curl_deduplicates_readiness_and_removal) {
  CurlNetworkWait wait;
  wait.ready(1);
  wait.ready(1);
  wait.consume(1); // also used when libcurl removes a queued socket
  wait.begin();
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  wait.end();
  EXPECT_GE(wait.elapsed_ns(), 5'000'000);
}

} // namespace
