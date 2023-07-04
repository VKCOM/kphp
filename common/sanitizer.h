// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#if defined(__SANITIZE_ADDRESS__)
# define ASAN_ENABLED 1
# include <sanitizer/asan_interface.h>
#elif defined(__has_feature)
# if __has_feature(address_sanitizer)
#  define ASAN_ENABLED 1
#  include <sanitizer/asan_interface.h>
# endif
#endif

#if !defined(ASAN_ENABLED)
# define ASAN_ENABLED 0
#endif

#if defined(__clang__)
# define ubsan_supp(x) __attribute__((no_sanitize(x)))
#else
# define ubsan_supp(x) __attribute__((no_sanitize_undefined))
#endif

#if !defined(USAN_ENABLED)
# define USAN_ENABLED 0
#endif