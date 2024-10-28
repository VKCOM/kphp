// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include "common/ucontext/ucontext-portable.h"

uint8_t ctx1_stack[8192];
uint8_t ctx2_stack[8192];

ucontext_t_portable cur_ctx;
ucontext_t_portable ctx1;
ucontext_t_portable ctx2;
ucontext_t_portable ctx3;
volatile int finished {0};

void ctx1_entrypoint(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
  ASSERT_EQ(a0, 0);
  ASSERT_EQ(a1, 1);
  ASSERT_EQ(a2, 2);
  ASSERT_EQ(a3, 3);
  ASSERT_EQ(a4, 4);
  ASSERT_EQ(a5, 5);
  ASSERT_EQ(a6, 6);
  ASSERT_EQ(a7, 7);
  ASSERT_EQ(swapcontext_portable(&ctx1, &ctx2), 0);
  finished = 1;
}

void ctx2_entrypoint() {
  ASSERT_EQ(swapcontext_portable(&ctx2, &ctx1), 0);
}

/*
  Current context -> swap to context 2 -> run entrypoint of context2 ->
  -> swap to context 1 -> run entrypoint of context 2 -> swap before `finished` is set to context 2 ->
  -> run trampoline of context 2 and swap to context 1 -> continue run entrypoint of context 1 and set `finished` to 1 ->
  -> run trampoline of context 1 and swap to initial context.
*/
TEST(ucontext_portable, swapcontext) {
  ASSERT_EQ(getcontext_portable(&ctx1), 0);
  set_context_stack_ptr_portable(ctx1, ctx1_stack);
  set_context_stack_size_portable(ctx1, sizeof(ctx1_stack));
  set_context_link_portable(ctx1, &cur_ctx);
  makecontext_portable(&ctx1, reinterpret_cast<void(*)()>(ctx1_entrypoint), 8, 0, 1, 2, 3, 4, 5, 6, 7);

  ASSERT_EQ(getcontext_portable(&ctx2), 0);
  set_context_stack_ptr_portable(ctx2, ctx2_stack);
  set_context_stack_size_portable(ctx2, sizeof(ctx2_stack));
  set_context_link_portable(ctx2, &ctx1);
  makecontext_portable(&ctx2, reinterpret_cast<void(*)()>(ctx2_entrypoint), 0);

  ASSERT_EQ(swapcontext_portable(&cur_ctx, &ctx2), 0);
  ASSERT_EQ(finished, 1);
}

/*
  Imitation of `do { i++; } while (i < 10)`
*/
volatile int context_set_cnt {0};
TEST(ucontext_portable, get_and_setcontext) {
  getcontext_portable(&ctx3);
  context_set_cnt = context_set_cnt + 1; // Due to `++` and `+=` ops with volatile are deprecated
  if (context_set_cnt < 10) {
    setcontext_portable(&ctx3);
  }
  ASSERT_EQ(context_set_cnt, 10);
}