// Compiler for PHP (aka KPHP)
// Copyright (c) 2018-2022 Ariadne Conill <ariadne@dereferenced.org>
// https://github.com/kaniini/libucontext/tree/master (copied as third-party and slightly modified)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

struct libucontext_mcontext {
  unsigned long fault_address;
  unsigned long regs[29];
  unsigned long fp, lr, sp, pc, pstate;
  long double __reserved[256];
};

struct libucontext_stack {
  void *ss_sp;
  int ss_flags;
  size_t ss_size;
};

struct libucontext_ucontext {
  unsigned long uc_flags;
  struct libucontext_ucontext *uc_link;
  libucontext_stack uc_stack;
  unsigned char __pad[136];
  libucontext_mcontext uc_mcontext;
};

extern "C" {
void libucontext_makecontext(libucontext_ucontext *, void (*)(), int, ...);
int libucontext_getcontext(libucontext_ucontext *);
int libucontext_setcontext(const libucontext_ucontext *);
int libucontext_swapcontext(libucontext_ucontext *, const libucontext_ucontext *);
}
