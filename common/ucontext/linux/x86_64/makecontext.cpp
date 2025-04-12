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

#include <cstdarg>
#include <cstdint>

#include "common/ucontext/linux/x86_64/context.h"

/* This implementation can handle any ARGC value but only
   normal integer parameters.
   makecontext sets up a stack and the registers for the
   user context. The stack looks like this:

    %rsp ->    +-----------------------+
               | trampoline address    |           / \
               +-----------------------+            |
               | parameter 7-n         |            |
               +-----------------------+            |
               | next context          |            |
               +-----------------------+   stack.sp + stack.size

   The registers are set up like this:
     %rdi,%rsi,%rdx,%rcx,%r8,%r9: parameter 1 to 6
     %rbx   : address of next context
     %rsp   : stack pointer.
*/

extern "C" void start_context_portable(void) __attribute__((visibility("hidden")));

extern "C" void makecontext_portable(kcontext_t* kcp, void (*func)(void), int argc, ...) {
  kgreg_t* sp;
  va_list ap;
  uint32_t argc_on_stack = (argc > 6 ? argc - 6 : 0); // Prev 6 args passed via regs
  uint32_t link = argc_on_stack + 1;
  int i;

  // Generate room on stack for parameter if needed and link
  sp = reinterpret_cast<kgreg_t*>(reinterpret_cast<uintptr_t>(kcp->stack.sp) + kcp->stack.size);
  sp -= argc_on_stack + 1;
  // Align stack and make space for trampoline address
  sp = reinterpret_cast<kgreg_t*>((reinterpret_cast<uintptr_t>(sp) & -16L) - 8);

  // Address to jump to context entrypoint
  kcp->mcontext.gregs[GREG_RIP] = reinterpret_cast<uintptr_t>(func);
  // Setup rbx
  kcp->mcontext.gregs[GREG_RBX] = reinterpret_cast<uintptr_t>(&sp[link]);
  kcp->mcontext.gregs[GREG_RSP] = reinterpret_cast<uintptr_t>(sp);

  /* Setup stack.  */
  sp[0] = reinterpret_cast<uintptr_t>(&start_context_portable);
  sp[link] = reinterpret_cast<uintptr_t>(kcp->link);

  va_start(ap, argc);
  /* Handle arguments. */
  for (i = 0; i < argc; ++i)
    switch (i) {
    case 0:
      kcp->mcontext.gregs[GREG_RDI] = va_arg(ap, kgreg_t);
      break;
    case 1:
      kcp->mcontext.gregs[GREG_RSI] = va_arg(ap, kgreg_t);
      break;
    case 2:
      kcp->mcontext.gregs[GREG_RDX] = va_arg(ap, kgreg_t);
      break;
    case 3:
      kcp->mcontext.gregs[GREG_RCX] = va_arg(ap, kgreg_t);
      break;
    case 4:
      kcp->mcontext.gregs[GREG_R8] = va_arg(ap, kgreg_t);
      break;
    case 5:
      kcp->mcontext.gregs[GREG_R9] = va_arg(ap, kgreg_t);
      break;
    default:
      /* Put value on stack.  */
      sp[i - 5] = va_arg(ap, kgreg_t);
      break;
    }
  va_end(ap);
}
