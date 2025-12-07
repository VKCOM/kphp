// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// #pragma GCC optimize("no-omit-frame-pointer")
// #define backtrace fast_backtrace

extern "C" {
extern void* __libc_stack_end;
extern char* stack_end;
}

int fast_backtrace(void** buffer, int size) __attribute__((noinline));
int fast_backtrace_by_bp(void* bp, void* stack_end_, void** buffer, int size) __attribute__((noinline));
int fast_backtrace_without_recursions(void** buffer, int size) noexcept;
int fast_backtrace_without_recursions_by_bp(void* bp, void* stack_end_, void** buffer, int size) noexcept;
