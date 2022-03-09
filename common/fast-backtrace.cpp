// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/fast-backtrace.h"

#include <algorithm>
#include <cassert>
#include <execinfo.h>

#include "common/sanitizer.h"

char *stack_end;

#if defined(__aarch64__) || defined(__APPLE__)
int fast_backtrace (void **buffer, int size) {
  return backtrace(buffer, size);
}
#elif defined(__x86_64__)
extern void *__libc_stack_end;

struct stack_frame {
  struct stack_frame *bp;
  void *ip;
};

static __inline__ void *get_bp () {
  void *bp;
  __asm__ volatile ("movq %%rbp, %[r]" : [r] "=r" (bp));
  return bp;
}

int fast_backtrace (void **buffer, int size) {
  if (!stack_end) {
    stack_end = static_cast<char *>(__libc_stack_end);
  }

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

#else
#error "Unsupported arch"
#endif

#if defined(__aarch64__) || defined(__APPLE__)
int fast_backtrace_without_recursions(void **, int) noexcept {
  return 0;
}
#else
int fast_backtrace_without_recursions(void **buffer, int size) noexcept {
  if (!stack_end) {
    stack_end = static_cast<char *>(__libc_stack_end);
  }

  auto *bp = static_cast<stack_frame *>(get_bp());
  int i = 0;
  while (i < size && reinterpret_cast<char *>(bp) <= stack_end && !(reinterpret_cast<long>(bp) & (sizeof(long) - 1))) {
    buffer[i++] = bp->ip;
    for (int possible_recursion_len = 1; i >= 2 * possible_recursion_len; ++possible_recursion_len) {
      if (std::equal(buffer + i - possible_recursion_len, buffer + i, buffer + i - possible_recursion_len * 2)) {
        i -= possible_recursion_len;
        break;
      }
    }

    stack_frame *p = bp->bp;
    if (p <= bp) {
      break;
    }
    bp = p;
  }
  return i;
}
#endif
