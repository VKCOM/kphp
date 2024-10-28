// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/ucontext/linux/x86_64/defs.h"

/*
 * int setcontext_portable(kcontext_t *kcp)

  Restores a user context in the same manner as in libc but without signals state restoring.
  Main goal of getting rid of manipulation with signals state -- remove syscall and performance gain.
  IMPORTANT! USER HAVE TO MANUALLY CONTROL SIGNALS STATE!

  In normal case it doesn't return, swap to passed context
*/

ENTRY(setcontext_portable)
    // Move the pointer into RDX. The choice is arbitrary, but
    // leaving RDI and RSI available for use later can avoid
    // shuffling values
    movq  %rdi, %rdx

    // Restore the floating-point context
    movq  oFPREGS(%rdx), %rcx
    fldenv  (%rcx)
    ldmxcsr oMXCSR(%rdx)

    // Load the new stack pointer, the preserved registers and registers used for passing args
    movq  oRSP(%rdx), %rsp
    movq  oRBX(%rdx), %rbx
    movq  oRBP(%rdx), %rbp
    movq  oR12(%rdx), %r12
    movq  oR13(%rdx), %r13
    movq  oR14(%rdx), %r14
    movq  oR15(%rdx), %r15

    // The following ret should return to the address set with getcontext. Therefore, push the address on the stack
    movq  oRIP(%rdx), %rcx
    pushq %rcx

    movq  oRSI(%rdx), %rsi
    movq  oRDI(%rdx), %rdi
    movq  oRCX(%rdx), %rcx
    movq  oR8(%rdx), %r8
    movq  oR9(%rdx), %r9
    movq  oRDX(%rdx), %rdx

    // Never returns, just for regularity
    xorl  %eax, %eax
    ret
END(setcontext_portable)
