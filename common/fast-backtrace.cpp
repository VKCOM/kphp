// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/fast-backtrace.h"

#include <algorithm>
#include <cassert>
#include <execinfo.h>

#include "common/sanitizer.h"

char *stack_end;

#ifndef __APPLE__
extern void *__libc_stack_end;
#endif

struct stack_frame {
  struct stack_frame *bp;
  void *ip;
};

static __inline__ void *get_bp() {
  void *result = nullptr;
#ifdef __x86_64__
  __asm__ volatile("movq %%rbp, %[r]" : [r] "=r"(result));
#elif defined(__aarch64__)
  __asm__ volatile("mov %0, fp" : "=r"(result));
#else
#error "Unsupported arch"
#endif
  return result;
}

int fast_backtrace(void **buffer, int size) {
#ifndef __APPLE__
  if (!stack_end) {
    stack_end = static_cast<char *>(__libc_stack_end);
  }
#endif

  auto *bp = static_cast<stack_frame *>(get_bp());
  int i = 0;
  while (i < size && reinterpret_cast<char *>(bp) <= stack_end && !(reinterpret_cast<long>(bp) & (sizeof(long) - 1))) {
    void *ip = bp->ip;
    buffer[i++] = ip;
    stack_frame *p = bp->bp;
    if (p <= bp) {
      break;
    }
    bp = p;
  }
  return i;
}

int fast_backtrace_without_recursions(void **buffer, int size) noexcept {
#ifndef __APPLE__
  if (!stack_end) {
    stack_end = static_cast<char *>(__libc_stack_end);
  }
#endif
  return fast_backtrace_without_recursions_by_bp(get_bp(), stack_end, buffer, size);
}

int fast_backtrace_without_recursions_by_bp(void *bp, void *stack_end_, void **buffer, int size) noexcept {
  stack_frame *current_bp = static_cast<stack_frame *>(bp);
  int i = 0;
  while (i < size && reinterpret_cast<char *>(current_bp) <= stack_end_ && !(reinterpret_cast<long>(current_bp) & (sizeof(long) - 1))) {
    buffer[i++] = current_bp->ip;
    for (int possible_recursion_len = 1; i >= 2 * possible_recursion_len; ++possible_recursion_len) {
      if (std::equal(buffer + i - possible_recursion_len, buffer + i, buffer + i - possible_recursion_len * 2)) {
        i -= possible_recursion_len;
        break;
      }
    }

    stack_frame *p = current_bp->bp;
    if (p <= current_bp) {
      break;
    }
    current_bp = p;
  }
  return i;
}
