#ifndef FAST_BACKTRACE_H__
#define FAST_BACKTRACE_H__

//#pragma GCC optimize("no-omit-frame-pointer")
//#define backtrace fast_backtrace

#ifdef __cplusplus
extern "C" {
#endif

extern void *__libc_stack_end;
extern char *stack_end;
int fast_backtrace (void **buffer, int size) __attribute__ ((noinline));

#ifdef __cplusplus
}
#endif


#endif
