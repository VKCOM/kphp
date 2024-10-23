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

inline constexpr void *get_context_stack_ptr_portable(const libucontext_ucontext &ctx) noexcept {
  return ctx.uc_stack.ss_sp;
}

inline constexpr size_t get_context_stack_size_portable(const libucontext_ucontext &ctx) noexcept {
  return ctx.uc_stack.ss_size;
}

inline constexpr void set_context_stack_ptr_portable(libucontext_ucontext &ctx, void *sp) noexcept {
  ctx.uc_stack.ss_sp = sp;
}

inline constexpr void set_context_stack_size_portable(libucontext_ucontext &ctx, size_t size) noexcept {
  ctx.uc_stack.ss_size = size;
}

inline constexpr void set_context_link_portable(libucontext_ucontext &ctx, libucontext_ucontext *link) noexcept {
  ctx.uc_link = link;
}

inline void *get_context_stack_base_ptr_portable(const libucontext_ucontext &ctx) noexcept {
  return reinterpret_cast<void *>(ctx.uc_mcontext.fp);
}