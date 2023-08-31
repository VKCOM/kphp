// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#if !(defined(__APPLE__) && defined(__arm64__))
// for x86 mac or x86/arm linux, we just use makecontext(), ucontext_t and other native functions
#include <ucontext.h>

#define ucontext_t_portable ucontext_t
#define setcontext_portable setcontext
#define getcontext_portable getcontext
#define makecontext_portable makecontext
#define swapcontext_portable swapcontext

#else
// for M1, we can't use native makecontext() and others: they compile, but hang up when called
#include "common/ucontext/ucontext.h"

#define ucontext_t_portable libucontext_ucontext
#define setcontext_portable libucontext_setcontext
#define getcontext_portable libucontext_getcontext
#define makecontext_portable libucontext_makecontext
#define swapcontext_portable libucontext_swapcontext

#endif
