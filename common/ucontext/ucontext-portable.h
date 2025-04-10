// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#pragma once

#include <cassert>

#ifdef __x86_64__
  #ifdef __linux__
    /*
     *
     * For X86_64 Linux platform we use own implementation of user context manipulation routines in the same manner as in libc.
     * But without signals state storing/restoring.
     * IMPORTANT! USER HAVE TO MANUALLY CONTROL SIGNALS STATE!
     *
     */
    #include "common/ucontext/linux/x86_64/context.h"
using ucontext_t_portable = kcontext_t;

extern "C" {
int getcontext_portable(ucontext_t_portable*);
void makecontext_portable(ucontext_t_portable*, void (*)(), int, ...);
int setcontext_portable(const ucontext_t_portable*);
int swapcontext_portable(ucontext_t_portable*, const ucontext_t_portable*);
}

  #elif __APPLE__
    #include "common/ucontext/darwin/x86_64/context.h"
using ucontext_t_portable = ucontext_t;

    #define getcontext_portable getcontext
    #define makecontext_portable makecontext
    #define setcontext_portable setcontext
    #define swapcontext_portable swapcontext

  #else
static_assert(false, "Unsupported OS for x86_64 platform");
  #endif

#elif defined(__aarch64__) || defined(__arm64__)

  #ifdef __linux__
    #include "common/ucontext/linux/aarch64/context.h"
using ucontext_t_portable = ucontext_t;

    #define getcontext_portable getcontext
    #define makecontext_portable makecontext
    #define setcontext_portable setcontext
    #define swapcontext_portable swapcontext

  #elif __APPLE__
    // For M1, we can't use native makecontext() and others: they compile, but hang up when called
    #include "common/ucontext/darwin/aarch64/context.h"
using ucontext_t_portable = libucontext_ucontext;

extern "C" {
void makecontext_portable(ucontext_t_portable*, void (*)(), int, ...);
int getcontext_portable(ucontext_t_portable*);
int setcontext_portable(const ucontext_t_portable*);
int swapcontext_portable(ucontext_t_portable*, const ucontext_t_portable*);
}
  #else
static_assert(false, "Unsupported OS for aarch64 platform");
  #endif

#endif
