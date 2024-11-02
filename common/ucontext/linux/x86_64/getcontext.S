//  Modified by LLC «V Kontakte», 2024 November 1
//
//  This file is part of the GNU C Library.
//  Copyright (C) 2002-2024 Free Software Foundation, Inc.
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "common/ucontext/linux/x86_64/defs.h"

/*
 * int getcontext_portable(kcontext_t *kcp)

  Stores a user context in the same manner as in libc but without signals state storing.
  Main goal of getting rid of manipulation with signals state -- remove syscall and performance gain.
  IMPORTANT! USER HAVE TO MANUALLY CONTROL SIGNALS STATE!

  In any case returns 0
*/

ENTRY(getcontext_portable)
    // Save the registers, the registers used for passing args, and the return address
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
    fldenv  (%rcx)
    stmxcsr oMXCSR(%rdi)

    // All done, return 0 for success
    xorl  %eax, %eax
    ret
END(getcontext_portable)
