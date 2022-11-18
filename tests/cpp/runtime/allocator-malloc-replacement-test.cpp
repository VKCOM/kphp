#include <functional>
#include <gtest/gtest.h>

#include "common/sanitizer.h"

#include "runtime/allocator.h"

TEST(allocator_malloc_replacement_test, test_replace_and_check) {
  ASSERT_FALSE(dl::is_malloc_replaced());
  dl::replace_malloc_with_script_allocator();
  ASSERT_TRUE(dl::is_malloc_replaced());
  dl::rollback_malloc_replacement();
  ASSERT_FALSE(dl::is_malloc_replaced());
}

TEST(allocator_malloc_replacement_test, test_replacement_raii) {
  ASSERT_FALSE(dl::is_malloc_replaced());

  {
    auto guard = make_malloc_replacement_with_script_allocator_guard();
    ASSERT_TRUE(dl::is_malloc_replaced());
  }

  ASSERT_FALSE(dl::is_malloc_replaced());

  {
    auto guard = make_malloc_replacement_with_script_allocator_guard(false);
    ASSERT_FALSE(dl::is_malloc_replaced());
  }

  ASSERT_FALSE(dl::is_malloc_replaced());
}

using MallocFn = std::function<void *(std::size_t)>;

static void do_malloc_fn_test(const MallocFn &malloc_fn) {
  ASSERT_FALSE(dl::is_malloc_replaced());

  const auto memory_stats_before = dl::get_script_memory_stats();
  {
    auto guard = make_malloc_replacement_with_script_allocator_guard();
    ASSERT_TRUE(dl::is_malloc_replaced());
    void *mem = malloc_fn(128);
    ASSERT_TRUE(mem);
#if !ASAN_ENABLED and !defined(__APPLE__)
    // asan doesn't work with our custom mallocs;
    // also custom malloc realization is platform specific and doesn't work on macos
    ASSERT_EQ(memory_stats_before.memory_used + 128 + 8, dl::get_script_memory_stats().memory_used);
#endif
    std::free(mem);
    ASSERT_EQ(memory_stats_before.memory_used, dl::get_script_memory_stats().memory_used);
  }

  ASSERT_FALSE(dl::is_malloc_replaced());
  ASSERT_EQ(memory_stats_before.memory_used, dl::get_script_memory_stats().memory_used);

  void *mem = malloc_fn(128);
  ASSERT_TRUE(mem);
  ASSERT_EQ(memory_stats_before.memory_used, dl::get_script_memory_stats().memory_used);
  std::free(mem);
}

TEST(allocator_malloc_replacement_test, test_malloc) {
  auto malloc_fn = [](std::size_t size) { return std::malloc(size); };
  do_malloc_fn_test(malloc_fn);
}

TEST(allocator_malloc_replacement_test, test_calloc) {
  auto malloc_fn = [](std::size_t size) { return std::calloc(1, size); };
  do_malloc_fn_test(malloc_fn);
}

TEST(allocator_malloc_replacement_test, test_realloc) {
  auto malloc_fn = [](std::size_t size) {
    auto *mem = std::malloc(size);
    return std::realloc(mem, size);
  };
  do_malloc_fn_test(malloc_fn);
}

TEST(allocator_malloc_replacement_test, aligned_alloc) {
  auto malloc_fn = [](std::size_t size) { return std::aligned_alloc(8, size); };
  do_malloc_fn_test(malloc_fn);
}

// macos lacks memalign
#if !defined(__APPLE__)
#include <malloc.h>

TEST(allocator_malloc_replacement_test, test_memalign) {
  auto malloc_fn = [](std::size_t size) { return memalign(8, size); };
  do_malloc_fn_test(malloc_fn);
}

#endif
