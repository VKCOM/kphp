// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#if !defined(__APPLE__)
// for x86, we just use makecontext(), ucontext_t and other native functions
#include <ucontext.h>

#define ucontext_t_portable ucontext_t
#define setcontext_portable setcontext
#define getcontext_portable getcontext
#define makecontext_portable makecontext
#define swapcontext_portable swapcontext

#else
// for M1, we can't use native makecontext() and others: they compile, but hang up when called
// instead, we require the ucontext library: https://github.com/kaniini/libucontext
// see the docs: https://vkcom.github.io/kphp/kphp-internals/developing-and-extending-kphp/compiling-kphp-from-sources.html
// here we expect, that libucontext include/ folder is copied into a default include path (to homebrew)
extern "C" {
#include <libucontext/libucontext.h>
}

#define ucontext_t_portable libucontext_ucontext_t
#define setcontext_portable libucontext_setcontext
#define getcontext_portable libucontext_getcontext
#define makecontext_portable libucontext_makecontext
#define swapcontext_portable libucontext_swapcontext

#endif
