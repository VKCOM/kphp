// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
# define ASAN_ENABLED 1
# include <sanitizer/asan_interface.h>
#else
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
