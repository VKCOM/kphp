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
