#pragma once

#include "runtime/kphp_core.h"
#include "runtime/context.h"

void print(script_context_t * ctx, const char *s, size_t s_len);

void print(script_context_t * ctx, const string &s);

inline void f$echo(script_context_t * ctx, const string& s) {
  print(ctx, s);
}

int64_t f$rand();