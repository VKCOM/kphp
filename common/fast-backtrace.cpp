// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/fast-backtrace.h"

#include <assert.h>
#include <execinfo.h>

#include "common/sanitizer.h"

char *stack_end;

#if defined(__x86_64__)
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

  struct stack_frame *bp = (struct stack_frame *)get_bp ();
  int i = 0;
  while (i < size && (char *) bp <= stack_end && !((long) bp & (sizeof (long) - 1))) {
    void *ip = bp->ip;
    buffer[i++] = ip;
    struct stack_frame *p = bp->bp;
    if (p <= bp) {
      break;
    }
    bp = p;
  }
  return i;
}
#elif defined(__aarch64__)

int fast_backtrace (void **buffer, int size) {
  return backtrace(buffer, size);
}

#else
#error "Unsupported arch"
#endif
