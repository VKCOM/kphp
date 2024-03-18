// Compiler for PHP (aka KPHP)
// libucontext (c) https://github.com/kaniini/libucontext/tree/master (copied as third-party and slightly modified)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/ucontext/ucontext-arm.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

enum { SP_OFFSET = 432, PC_OFFSET = 440, PSTATE_OFFSET = 448, FPSIMD_CONTEXT_OFFSET = 464 };

#define STR(x) #x
#ifdef __APPLE__
#define NAME(x) STR(_##x)
#else
#define NAME(x) #x
#endif

#define R0_OFFSET REG_OFFSET(0)

#define REG_OFFSET(__reg) (MCONTEXT_GREGS + ((__reg)*REG_SZ))

#define REG_SZ (8)
#define MCONTEXT_GREGS (184)

static_assert(offsetof(libucontext_ucontext, uc_mcontext.regs[0]) == R0_OFFSET, "R0_OFFSET is invalid");
static_assert(offsetof(libucontext_ucontext, uc_mcontext.sp) == SP_OFFSET, "SP_OFFSET is invalid");
static_assert(offsetof(libucontext_ucontext, uc_mcontext.pc) == PC_OFFSET, "PC_OFFSET is invalid");
static_assert(offsetof(libucontext_ucontext, uc_mcontext.pstate) == PSTATE_OFFSET, "PSTATE_OFFSET is invalid");

__attribute__((visibility("hidden"))) void libucontext_trampoline() {
  libucontext_ucontext *uc_link = nullptr;

  asm("mov	%0, x19" : "=r"((uc_link)));

  if (uc_link == nullptr) {
    exit(0);
  }

  libucontext_setcontext(uc_link);
}

void libucontext_makecontext(libucontext_ucontext *ucp, void (*func)(), int argc, ...) {
  unsigned long *sp;
  unsigned long *regp;
  va_list va;
  int i;

  sp = reinterpret_cast<unsigned long *>(reinterpret_cast<uintptr_t>(ucp->uc_stack.ss_sp) + ucp->uc_stack.ss_size);
  sp -= argc < 8 ? 0 : argc - 8;
  sp = reinterpret_cast<unsigned long *>(((reinterpret_cast<uintptr_t>(sp) & -16L)));

  ucp->uc_mcontext.sp = reinterpret_cast<uintptr_t>(sp);
  ucp->uc_mcontext.pc = reinterpret_cast<uintptr_t>(func);
  ucp->uc_mcontext.regs[19] = reinterpret_cast<uintptr_t>(ucp->uc_link);
  ucp->uc_mcontext.lr = reinterpret_cast<uintptr_t>(&libucontext_trampoline);

  va_start(va, argc);

  regp = &(ucp->uc_mcontext.regs[0]);

  for (i = 0; (i < argc && i < 8); i++) {
    *regp++ = va_arg(va, unsigned long);
  }

  for (; i < argc; i++) {
    *sp++ = va_arg(va, unsigned long);
  }

  va_end(va);
}

asm(".global " NAME(libucontext_getcontext) ";\n"
                                            ".align  2;\n" NAME(libucontext_getcontext) ":\n"
                                                                                        "str	xzr, [x0, #((184) + ((0) * (8)))]\n" // #REG_OFFSET(0)
                                                                                        /* save GPRs */
                                                                                        "stp	x0, x1,   [x0, #((184) + ((0) * (8)))]\n"  // REG_OFFSET(0)
                                                                                        "stp	x2, x3,   [x0, #((184) + ((2) * (8)))]\n"  // REG_OFFSET(2)
                                                                                        "stp	x4, x5,   [x0, #((184) + ((4) * (8)))]\n"  // REG_OFFSET(4)
                                                                                        "stp	x6, x7,   [x0, #((184) + ((6) * (8)))]\n"  // REG_OFFSET(6)
                                                                                        "stp	x8, x9,   [x0, #((184) + ((8) * (8)))]\n"  // REG_OFFSET(8)
                                                                                        "stp	x10, x11, [x0, #((184) + ((10) * (8)))]\n" // REG_OFFSET(10)
                                                                                        "stp	x12, x13, [x0, #((184) + ((12) * (8)))]\n" // REG_OFFSET(12)
                                                                                        "stp	x14, x15, [x0, #((184) + ((14) * (8)))]\n" // REG_OFFSET(14)
                                                                                        "stp	x16, x17, [x0, #((184) + ((16) * (8)))]\n" // REG_OFFSET(16)
                                                                                        "stp	x18, x19, [x0, #((184) + ((18) * (8)))]\n" // REG_OFFSET(18)
                                                                                        "stp	x20, x21, [x0, #((184) + ((20) * (8)))]\n" // REG_OFFSET(20)
                                                                                        "stp	x22, x23, [x0, #((184) + ((22) * (8)))]\n" // REG_OFFSET(22)
                                                                                        "stp	x24, x25, [x0, #((184) + ((24) * (8)))]\n" // REG_OFFSET(24)
                                                                                        "stp	x26, x27, [x0, #((184) + ((26) * (8)))]\n" // REG_OFFSET(26)
                                                                                        "stp	x28, x29, [x0, #((184) + ((28) * (8)))]\n" // REG_OFFSET(28)
                                                                                        "str	x30,      [x0, #((184) + ((30) * (8)))]\n" // REG_OFFSET(30)
                                                                                        /* save current program counter in link register */
                                                                                        "str	x30, [x0, #440]\n" // PC_OFFSET
                                                                                        /* save current stack pointer */
                                                                                        "mov	x2, sp\n"
                                                                                        "str	x2, [x0, #432]\n" // SP_OFFSET
                                                                                        /* save pstate */
                                                                                        "str	xzr, [x0, #448]\n" // PSTATE_OFFSET
                                                                                        "add x2, x0,   #464\n"   // FPSIMD_CONTEXT_OFFSET
                                                                                        "stp q8, q9,   [x2, #144]\n"
                                                                                        "stp q10, q11, [x2, #176]\n"
                                                                                        "stp q12, q13, [x2, #208]\n"
                                                                                        "stp q14, q15, [x2, #240]\n"
                                                                                        "mov	x0, #0\n"
                                                                                        "ret\n");

asm(".global " NAME(libucontext_setcontext) ";\n"
                                            ".align  2;\n" NAME(libucontext_setcontext) ":\n"
                                                                                        /* restore GPRs */
                                                                                        "ldp	x18, x19, [x0, #((184) + ((18) * (8)))]\n" // REG_OFFSET(18)
                                                                                        "ldp	x20, x21, [x0, #((184) + ((20) * (8)))]\n" // REG_OFFSET(20)
                                                                                        "ldp	x22, x23, [x0, #((184) + ((22) * (8)))]\n" // REG_OFFSET(22)
                                                                                        "ldp	x24, x25, [x0, #((184) + ((24) * (8)))]\n" // REG_OFFSET(24)
                                                                                        "ldp	x26, x27, [x0, #((184) + ((26) * (8)))]\n" // REG_OFFSET(26)
                                                                                        "ldp	x28, x29, [x0, #((184) + ((28) * (8)))]\n" // REG_OFFSET(28)
                                                                                        "ldr	x30,      [x0, #((184) + ((30) * (8)))]\n" // REG_OFFSET(30)
                                                                                        /* save current stack pointer */
                                                                                        "ldr	x2, [x0, #432]\n" // SP_OFFSET
                                                                                        "mov	sp, x2\n"
                                                                                        "add x2, x0, #464\n" // FPSIMD_CONTEXT_OFFSET
                                                                                        "ldp q8, q9,   [x2, #144]\n"
                                                                                        "ldp q10, q11, [x2, #176]\n"
                                                                                        "ldp q12, q13, [x2, #208]\n"
                                                                                        "ldp q14, q15, [x2, #240]\n"
                                                                                        /* save current program counter in link register */
                                                                                        "ldr	x16, [x0, #440]\n" // PC_OFFSET
                                                                                        /* restore args */
                                                                                        "ldp	x2, x3, [x0, #((184) + ((2) * (8)))]\n" // REG_OFFSET(2)
                                                                                        "ldp	x4, x5, [x0, #((184) + ((4) * (8)))]\n" // REG_OFFSET(4)
                                                                                        "ldp	x6, x7, [x0, #((184) + ((6) * (8)))]\n" // REG_OFFSET(6)
                                                                                        "ldp	x0, x1, [x0, #((184) + ((0) * (8)))]\n" // REG_OFFSET(8)
                                                                                        /* jump to new PC */
                                                                                        "br	x16\n");

asm(".global " NAME(libucontext_swapcontext) ";\n"
                                             ".align  2;\n" NAME(
                                               libucontext_swapcontext) ":\n"
                                                                        "str	xzr, [x0, #((184) + ((0) * (8)))]\n" // REG_OFFSET(0)
                                                                        /* save GPRs */
                                                                        "stp	x2, x3,   [x0, #((184) + ((2) * (8)))]\n"  // REG_OFFSET(2)
                                                                        "stp	x4, x5,   [x0, #((184) + ((4) * (8)))]\n"  // REG_OFFSET(4)
                                                                        "stp	x6, x7,   [x0, #((184) + ((6) * (8)))]\n"  // REG_OFFSET(6)
                                                                        "stp	x8, x9,   [x0, #((184) + ((8) * (8)))]\n"  // REG_OFFSET(8)
                                                                        "stp	x10, x11, [x0, #((184) + ((10) * (8)))]\n" // REG_OFFSET(10)
                                                                        "stp	x12, x13, [x0, #((184) + ((12) * (8)))]\n" // REG_OFFSET(12)
                                                                        "stp	x14, x15, [x0, #((184) + ((14) * (8)))]\n" // REG_OFFSET(14)
                                                                        "stp	x16, x17, [x0, #((184) + ((16) * (8)))]\n" // REG_OFFSET(16)
                                                                        "stp	x18, x19, [x0, #((184) + ((18) * (8)))]\n" // REG_OFFSET(18)
                                                                        "stp	x20, x21, [x0, #((184) + ((20) * (8)))]\n" // REG_OFFSET(20)
                                                                        "stp	x22, x23, [x0, #((184) + ((22) * (8)))]\n" // REG_OFFSET(22)
                                                                        "stp	x24, x25, [x0, #((184) + ((24) * (8)))]\n" // REG_OFFSET(24)
                                                                        "stp	x26, x27, [x0, #((184) + ((26) * (8)))]\n" // REG_OFFSET(26)
                                                                        "stp	x28, x29, [x0, #((184) + ((28) * (8)))]\n" // REG_OFFSET(28)
                                                                        "str	x30,      [x0, #((184) + ((30) * (8)))]\n" // REG_OFFSET(30)
                                                                        /* save current program counter in link register */
                                                                        "str	x30, [x0, #440]\n" // PC_OFFSET
                                                                        /* save current stack pointer */
                                                                        "mov	x2, sp\n"
                                                                        "str	x2, [x0, #432]\n" // SP_OFFSET
                                                                        /* save pstate */
                                                                        "str	xzr, [x0, #448]\n" // PSTATE_OFFSET
                                                                        "add x2, x0,    #464\n"  // FPSIMD_CONTEXT_OFFSET
                                                                        "stp q8, q9,   [x2, #144]\n"
                                                                        "stp q10, q11, [x2, #176]\n"
                                                                        "stp q12, q13, [x2, #208]\n"
                                                                        "stp q14, q15, [x2, #240]\n"
                                                                        /* context to swap to is in x1 so... we move to x0 and call setcontext */
                                                                        /* store our link register in x28 */
                                                                        "mov	x28, x30\n"
                                                                        /* move x1 to x0 and call setcontext */
                                                                        "mov	x0, x1\n"
                                                                        "bl " NAME(libucontext_setcontext) "\n"
                                                                                                           /* hmm, we came back here try to return */
                                                                                                           "mov	x30, x28\n"
                                                                                                           "ret\n");
