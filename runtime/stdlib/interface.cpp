#include "runtime/stdlib/interface.h"

#include <random>

void print(script_context_t * ctx, const char *s, size_t s_len) {
  ctx->output_buffer.append(s, s_len);
}

void print(script_context_t * ctx, const string &s) {
  print(ctx, s.c_str(), s.size());
}

int64_t f$rand() {
  std::random_device rd;
  int64_t dice_roll = rd();
  return dice_roll;
}