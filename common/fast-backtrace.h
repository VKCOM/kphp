#ifndef FAST_BACKTRACE_H__
#define FAST_BACKTRACE_H__

//#pragma GCC optimize("no-omit-frame-pointer")
//#define backtrace fast_backtrace

extern "C" {
extern void *__libc_stack_end;
extern char *stack_end;
}

int fast_backtrace (void **buffer, int size) __attribute__ ((noinline));

#endif
