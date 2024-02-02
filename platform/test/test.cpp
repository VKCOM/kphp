#include <gtest/gtest.h>

#include <thread>
#include "runtime/context.h"

extern void run_script(script_context_t *ctx) noexcept;

TEST(platform_test, multithreading) {
  constexpr int THREADS_NUM = 12;

  std::vector<std::string> output(THREADS_NUM);
  auto thread_job = [&output](int id){
    script_context_t ctx;
    run_script(&ctx);
    output[id] = std::string(ctx.output_buffer.c_str());
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < THREADS_NUM; ++i) {
    threads.emplace_back(thread_job, i);
  }

  for (int i = 0; i < THREADS_NUM; ++i) {
    threads[i].join();
  }

  for (int i = 0; i < THREADS_NUM; ++i) {
    ASSERT_EQ(output[i], std::string("string"));
  }
}
