// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/ucontext/linux/x86_64/defs.h"

/*
 * int swapcontext_portable(kcontext_t *out_kcp, kcontext_t *kcp)

  Stores a user context into `out_kcp` and restores context from `kcp` in the same manner as in libc.
  But without signals state storing/restoring.
  Main goal of getting rid of manipulation with signals state -- remove syscall and performance gain.
  IMPORTANT! USER HAVE TO MANUALLY CONTROL SIGNALS STATE!

  In any case returns 0
*/

ENTRY(swapcontext_portable)
    // Save the preserved registers, the registers used for passing args, and the return address
    movq  %rbx, oRBX(%rdi)
    movq  %rbp, oRBP(%rdi)
    movq  %r12, oR12(%rdi)
    movq  %r13, oR13(%rdi)
    movq  %r14, oR14(%rdi)
    movq  %r15, oR15(%rdi)
    movq  %rdi, oRDI(%rdi)
    movq  %rsi, oRSI(%rdi)
    movq  %rdx, oRDX(%rdi)
    movq  %rcx, oRCX(%rdi)
    movq  %r8, oR8(%rdi)
    movq  %r9, oR9(%rdi)
    /*
        %rsp ->   +-----------------------+      / \
                  | Return address        |       |
     8(%rsp) ->   +-----------------------+       |
                  | Caller local var #N   |       |
                  +-----------------------+       |
                  |          ...          |       |
                  +-----------------------+   0xFFFFFFFF
    */
    movq  (%rsp), %rcx
    movq  %rcx, oRIP(%rdi)
    leaq  8(%rsp), %rcx
    movq  %rcx, oRSP(%rdi)

    // We have separate floating-point register content memory on the
    // stack.  We use the fpregs_mem block in the context
    leaq  oFPREGSMEM(%rdi), %rcx
    movq  %rcx, oFPREGS(%rdi)
    // Save the floating-point environment
    fnstenv (%rcx)
    stmxcsr oMXCSR(%rdi)

    // Move restored ctx pointer to %rdx
    movq  %rsi, %rdx

    // Restore the floating-point context
    movq  oFPREGS(%rdx), %rcx
    fldenv  (%rcx)
    ldmxcsr oMXCSR(%rdx)

    // Load the new stack pointer and the preserved registers
    movq  oRSP(%rdx), %rsp
    movq  oRBX(%rdx), %rbx
    movq  oRBP(%rdx), %rbp
    movq  oR12(%rdx), %r12
    movq  oR13(%rdx), %r13
    movq  oR14(%rdx), %r14
    movq  oR15(%rdx), %r15


    // The following ret should return to the address set with
    // getcontext.  Therefore. push the address on the stack
    movq  oRIP(%rdx), %rcx
    pushq %rcx

    // Setup registers used for passing args
    movq  oRDI(%rdx), %rdi
    movq  oRSI(%rdx), %rsi
    movq  oRCX(%rdx), %rcx
    movq  oR8(%rdx), %r8
    movq  oR9(%rdx), %r9
    movq  oRDX(%rdx), %rdx

    // Clear rax to indicate success
    xorl  %eax, %eax
    ret
END(swapcontext_portable)
