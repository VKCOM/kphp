#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "extra-memory-pool.h"
#include "monotonic_buffer_resource.h"
#include "unsynchronized_pool_resource.h"

void memory_resource::monotonic_buffer_resource::raise_oom(size_t) const noexcept {};
void memory_resource::monotonic_buffer_resource::critical_dump(void*, size_t) const noexcept {};

void php_assert__(const char* msg, const char* file, int line) {
  std::cout << "Assertion '" << msg << "' failed in file " << file << " on line" << line << "\n";
  exit(1);
}

constexpr size_t RAW_BUFFER_SIZE{1UL * 1024 * 1024}; // 1MiB
constexpr size_t MAX_BUFFERS_NUM{10};
uint8_t RAW_FLAT_BUFFER[MAX_BUFFERS_NUM * RAW_BUFFER_SIZE];
uint8_t RAW_MULTIPLE_BUFFERS[MAX_BUFFERS_NUM][RAW_BUFFER_SIZE];

struct SingleBufferMemoryManager {
  explicit SingleBufferMemoryManager() noexcept {
    pool.init(RAW_FLAT_BUFFER, MAX_BUFFERS_NUM * RAW_BUFFER_SIZE);
  }

  void* alloc(size_t size) noexcept {
    auto* mem{reinterpret_cast<size_t*>(pool.allocate(sizeof(size_t) + size))};
    if (mem == nullptr) {
      return mem;
    }
    *(mem) = sizeof(size_t) + size;
    return mem + 1;
  }

  void dealloc(void* ptr) noexcept {
    auto* mem{reinterpret_cast<size_t*>(ptr) - 1};
    const size_t size{*mem};
    pool.deallocate(mem, size);
  }

  memory_resource::unsynchronized_pool_resource pool{};
};

struct MultipleBufferMemoryManager {
  explicit MultipleBufferMemoryManager() noexcept {
    pool.init(RAW_MULTIPLE_BUFFERS[0], RAW_BUFFER_SIZE);
  }

  void* alloc(size_t size) noexcept {
    auto* mem{reinterpret_cast<size_t*>(pool.allocate(sizeof(size_t) + size))};
    if (mem == nullptr) {
      current_buffer_id += 1;
      if (current_buffer_id > MAX_BUFFERS_NUM) {
        std::cout << "MultipleBufferMemoryManager: not enough buffers for extra pool" << "\n";
        return nullptr;
      }
      pool.add_extra_memory(new (RAW_MULTIPLE_BUFFERS[current_buffer_id]) memory_resource::extra_memory_pool{RAW_BUFFER_SIZE});
      mem = reinterpret_cast<size_t*>(pool.allocate(sizeof(size_t) + size));
      if (mem == nullptr) {
        std::cout << "MultipleBufferMemoryManager: not enough extra memory" << "\n";
        return nullptr;
      }
    }
    *(mem) = sizeof(size_t) + size;
    return mem + 1;
  }

  void dealloc(void* ptr) noexcept {
    auto* mem{reinterpret_cast<size_t*>(ptr) - 1};
    const size_t size{*mem};
    pool.deallocate(mem, size);
  }

  memory_resource::unsynchronized_pool_resource pool{};
  uint32_t current_buffer_id{0};
};

void simple_alloc_single_buffer() {
  using clock = std::chrono::steady_clock;

  SingleBufferMemoryManager mm{};

  auto start{clock::now()};
  for (size_t i = 0; i < 655360; ++i) {
    mm.alloc(8);
  }

  auto elapsed{std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - start).count()};
  std::cout << "Elapsed: " << elapsed << "\n";
}

void simple_alloc_multiple_buffer() {
  using clock = std::chrono::steady_clock;

  MultipleBufferMemoryManager mm{};

  auto start{clock::now()};
  for (size_t i = 0; i < 655360; ++i) {
    mm.alloc(8);
  }

  auto elapsed{std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - start).count()};
  std::cout << "Elapsed: " << elapsed << "\n";
}

int main() {
  // simple_alloc_single_buffer();
  simple_alloc_multiple_buffer();
}
