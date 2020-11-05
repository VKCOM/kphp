// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdbool.h>

// The above is a work around GCC 4.9 implementation bug: the header below uses `bool` but doesn't include <stdbool.h> in C mode.
#include <sanitizer/asan_interface.h>

#if defined(__SANITIZE_ADDRESS__)
# define ASAN_ENABLED 1
#elif defined(__has_feature)
# if __has_feature(address_sanitizer)
#  define ASAN_ENABLED 1
# endif
#endif

#if !defined(ASAN_ENABLED)
# define ASAN_ENABLED 0
#elif __GNUC__ >= 7
// gcc prior 7 version didn't have __sanitizer_start[finish]_switch_fiber to control stack pointer during swapcontext
// some functions(fast_backtrace) cause stack-buffer-overflow due to stack traversing out of frame, nevertheless it's ok for sanitizer
// asan starts to produce false positive overflow due to swapcontext with c++ exceptions (if you have non-trivial destructors)
# define ASAN7_ENABLED 1
#endif

#if defined(__clang__)
  #define ubsan_supp(x) __attribute__((no_sanitize(x)))
#else
  #define ubsan_supp(x) __attribute__((no_sanitize_undefined))
#endif

#if !defined(USAN_ENABLED)
# define USAN_ENABLED 0
#endif
