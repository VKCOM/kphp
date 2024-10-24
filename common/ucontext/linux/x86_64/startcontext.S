#include "defs.h"

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
