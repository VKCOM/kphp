#include <cstddef>
#include <cstdint>
#include "defs.h"

#pragma once

// Type for general register
using kgreg_t = uint64_t;

// Container for all general registers
using kgregset_t = kgreg_t[NUM_GENERAL_REG];

// Prohibit packing
#pragma pack(push, 1)

struct kfpxreg {
  uint16_t significand[4];
  uint16_t exponent;
  uint16_t reserved1[3];
};

struct kxmmreg {
  uint32_t element[4];
};

struct kfpstate {
  uint16_t cwd;
  uint16_t swd;
  uint16_t ftw;
  uint16_t fop;
  uint16_t rip;
  uint16_t rdp;
  uint16_t mxcsr;
  uint16_t mxcr_mask;
  kfpxreg st[8];
  kxmmreg xmm[16];
  uint32_t reserved1[24];
};

// Structure to describe FPU registers
using kfpregset_t = kfpstate *;

// Context to describe whole processor state
struct kmcontext_t {
  kgregset_t gregs;
  // Note that fpregs is a pointer
  kfpregset_t fpregs;
  uint64_t reserved1[8];
};

struct kstack_t {
  void *sp;
  uint64_t size;
};

// Userlevel KPHP context
struct kcontext_t {
  uint64_t flags;
  kcontext_t *link;
  kstack_t stack;
  kmcontext_t mcontext;
  kfpstate fpregs_mem;
};

#pragma pack(pop)

inline constexpr void *get_context_stack_ptr_portable(const kcontext_t &ctx) noexcept {
  return ctx.stack.sp;
}

inline constexpr uint64_t get_context_stack_size_portable(const kcontext_t &ctx) noexcept {
  return ctx.stack.size;
}

inline constexpr void set_context_stack_ptr_portable(kcontext_t &ctx, void *sp) noexcept {
  ctx.stack.sp = sp;
}

inline constexpr void set_context_stack_size_portable(kcontext_t &ctx, uint64_t size) noexcept {
  ctx.stack.size = size;
}

inline constexpr void set_context_link_portable(kcontext_t &ctx, kcontext_t *link) noexcept {
  ctx.link = link;
}

inline void *get_context_stack_base_ptr_portable(const kcontext_t &ctx) noexcept {
  return reinterpret_cast<void *>(ctx.mcontext.gregs[GREG_RBP]);
}

// Zero-cost offsets double-checking
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RBP]) == oRBP, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RSP]) == oRSP, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RBX]) == oRBX, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R8]) == oR8, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R9]) == oR9, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R10]) == oR10, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R11]) == oR11, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R12]) == oR12, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R13]) == oR13, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R14]) == oR14, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_R15]) == oR15, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RDI]) == oRDI, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RSI]) == oRSI, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RDX]) == oRDX, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RAX]) == oRAX, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RCX]) == oRCX, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_RIP]) == oRIP, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs[GREG_EFL]) == oEFL, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.gregs) == KCONTEXT_MCONTEXT_GREGS, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, mcontext.fpregs) == KCONTEXT_MCONTEXT_FPREGS, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, fpregs_mem) == KCONTEXT_FPREGS_MEM, "Invalid offset assumption");
static_assert(offsetof(kcontext_t, fpregs_mem.mxcsr) == KCONTEXT_FPREGS_MEM_MXCSR, "Invalid offset assumption");
