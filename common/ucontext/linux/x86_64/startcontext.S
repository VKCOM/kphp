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
 * void start_context_portable()
  The trampoline for a `next` context which is located in `link` of kcontext_t.
  It is placed into user-created stack by `makecontext`.

   %rsp ->    +---------------------------+
              | &start_context_portable   |          / \
              +---------------------------+           |
              | entrypoint parameter 7-n  |           |
   %rbx ->    +---------------------------+           |
              | next context              |           |
              +---------------------------+  stack.sp + stack.size
*/

ENTRY(start_context_portable)
    // This removes the parameters passed to the function given to
    // 'makecontext' from the stack.  RBX contains the address
    // on the stack pointer for the next context.  */
    movq  %rbx, %rsp

    // Don't use pop here so that stack is aligned to 16 bytes
    movq  (%rsp), %rdi    // This is the next context
    testq %rdi, %rdi
    je  2f                // If it is null then exit
    call  setcontext_portable
2:
    call exit
END(start_context_portable)
