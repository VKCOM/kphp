#pragma once

#include <cmath>

struct CoroutineStack {
  CoroutineStack();
  ~CoroutineStack();
  void * push(std::size_t n);
  void pop(void * ptr, std::size_t n);
  void resize(int64_t new_capacity);
  char * start;
  int64_t size;
  int64_t capacity;
};
