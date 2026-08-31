//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <memory>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "common/containers/object-pool.h"

namespace {

template<typename T>
struct counting_allocator : std::allocator<T> {
  using std::allocator<T>::allocator;

  static inline size_t allocate_calls{0};
  static inline size_t deallocate_calls{0};

  auto allocate(std::size_t n) -> T* {
    ++allocate_calls;
    return std::allocator<T>::allocate(n);
  }

  auto deallocate(T* p, std::size_t n) -> void {
    ++deallocate_calls;
    std::allocator<T>::deallocate(p, n);
  }
};

template<typename T>
struct tagged_allocator : std::allocator<T> {
  using std::allocator<T>::allocator;

  static inline std::vector<int> allocation_tags;

  int m_tag{0};

  tagged_allocator() = default;
  explicit tagged_allocator(int tag) noexcept
      : m_tag{tag} {}

  auto allocate(std::size_t n) -> T* {
    allocation_tags.push_back(m_tag);
    return std::allocator<T>::allocate(n);
  }
};

struct point {
  int x{};
  int y{};

  point() = default;
  point(int x_val, int y_val) noexcept
      : x{x_val},
        y{y_val} {}
};

struct dtor_tracker {
  bool* m_destroyed{nullptr};

  explicit dtor_tracker(bool* destroyed) noexcept
      : m_destroyed{destroyed} {}

  ~dtor_tracker() {
    *m_destroyed = true;
  }
};

} // namespace

TEST(object_pool_test, acquire_forward_no_constructor_arguments) {
  vk::object_pool<int, std::allocator> pool;
  pool.init(4);

  int& x{pool.acquire()};

  ASSERT_EQ(x, 0);

  pool.release(x);
}

TEST(object_pool_test, acquire_forwards_single_constructor_argument) {
  vk::object_pool<int, std::allocator> pool;
  pool.init(4);

  int& x{pool.acquire(42)};

  ASSERT_EQ(x, 42);

  pool.release(x);
}

TEST(object_pool_test, acquire_forwards_multiple_constructor_arguments) {
  vk::object_pool<point, std::allocator> pool;
  pool.init(4);

  point& p{pool.acquire(1, 2)};

  ASSERT_EQ(p.x, 1);
  ASSERT_EQ(p.y, 2);

  pool.release(p);
}

TEST(object_pool_test, acquire_forwards_move_only_arguments) {
  vk::object_pool<std::unique_ptr<int>, std::allocator> pool;
  pool.init(4);

  std::unique_ptr<int>& obj{pool.acquire(std::make_unique<int>(5))};

  ASSERT_NE(obj, nullptr);
  ASSERT_EQ(*obj, 5);

  pool.release(obj);
}

TEST(object_pool_test, release_invokes_destructor) {
  vk::object_pool<dtor_tracker, std::allocator> pool;
  pool.init(4);
  bool destroyed{false};

  dtor_tracker& obj{pool.acquire(&destroyed)};

  ASSERT_FALSE(destroyed);

  pool.release(obj);

  ASSERT_TRUE(destroyed);
}

TEST(object_pool_test, acquire_after_release_reuses_last_freed_slot) {
  vk::object_pool<int, std::allocator> pool;
  pool.init(4);

  int& a{pool.acquire(1)};
  int* addr_a{std::addressof(a)};
  pool.release(a);

  int& b{pool.acquire(2)};

  ASSERT_EQ(std::addressof(b), addr_a);

  pool.release(b);
}

TEST(object_pool_test, constructors_do_not_allocate_before_init) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> default_constructed;

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 0);

  vk::object_pool<int, counting_allocator> explicit_allocator_constructed{counting_allocator<std::byte>{}};

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 0);
}

TEST(object_pool_test, init_allocates_exactly_one_chunk) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> pool;
  pool.init(3);

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 1);

  int& x{pool.acquire(7)};
  pool.release(x);
}

TEST(object_pool_test, explicit_allocator_constructor_then_init_allocates_exactly_one_chunk) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> pool{counting_allocator<std::byte>{}};
  pool.init(3);

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 1);

  int& x{pool.acquire(7)};
  pool.release(x);
}

TEST(object_pool_test, explicit_allocator_constructor_stores_the_passed_allocator_instance) {
  tagged_allocator<std::byte>::allocation_tags.clear();

  vk::object_pool<int, tagged_allocator> pool{tagged_allocator<std::byte>{42}};
  pool.init(2);

  std::vector<int*> ptrs;
  for (int i = 0; i < 5; ++i) {
    ptrs.push_back(std::addressof(pool.acquire(i)));
  }

  ASSERT_FALSE(tagged_allocator<std::byte>::allocation_tags.empty());
  for (int tag : tagged_allocator<std::byte>::allocation_tags) {
    ASSERT_EQ(tag, 42);
  }

  for (auto* p : ptrs) {
    pool.release(*p);
  }
}

TEST(object_pool_test, acquiring_exactly_chunk_size_objects_does_not_grow) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> pool;
  pool.init(4);
  std::vector<int*> ptrs;
  for (int i = 0; i < 4; ++i) {
    ptrs.push_back(std::addressof(pool.acquire(i)));
  }

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 1);

  for (auto* p : ptrs) {
    pool.release(*p);
  }
}

TEST(object_pool_test, release_then_acquire_does_not_allocate_a_new_chunk) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> pool;
  pool.init(2);
  int& a{pool.acquire(1)};
  int& b{pool.acquire(2)};

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 1);

  pool.release(a);
  int& c{pool.acquire(3)};

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 1);

  pool.release(b);
  pool.release(c);
}

TEST(object_pool_test, allocates_chunks_when_exchausted) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  vk::object_pool<int, counting_allocator> pool;
  pool.init(2);
  std::vector<int*> ptrs;
  for (int i = 0; i < 5; ++i) {
    ptrs.push_back(std::addressof(pool.acquire(i)));
  }

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, 3);

  for (auto* p : ptrs) {
    pool.release(*p);
  }
}

TEST(object_pool_test, destructor_deallocates_every_allocated_chunk) {
  counting_allocator<std::byte>::allocate_calls = 0;
  counting_allocator<std::byte>::deallocate_calls = 0;

  {
    vk::object_pool<int, counting_allocator> pool;
    pool.init(2);
    std::vector<int*> ptrs;
    for (int i = 0; i < 5; ++i) {
      ptrs.push_back(std::addressof(pool.acquire(i)));
    }
    for (auto* p : ptrs) {
      pool.release(*p);
    }
  }

  ASSERT_EQ(counting_allocator<std::byte>::allocate_calls, counting_allocator<std::byte>::deallocate_calls);
  ASSERT_GE(counting_allocator<std::byte>::allocate_calls, 3);
}

TEST(object_pool_test, acquires_returns_distinct_objects) {
  constexpr int total{10};
  vk::object_pool<int, std::allocator> pool;
  pool.init(2);
  std::vector<int*> ptrs;

  for (int i = 0; i < total; ++i) {
    ptrs.push_back(std::addressof(pool.acquire(i)));
  }

  std::set<int*> unique_ptrs(ptrs.begin(), ptrs.end());
  ASSERT_EQ(unique_ptrs.size(), total);

  for (int i = 0; i < total; ++i) {
    ASSERT_EQ(*ptrs[i], i);
  }

  for (auto* p : ptrs) {
    pool.release(*p);
  }
}
