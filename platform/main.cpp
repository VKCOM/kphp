#include "runtime/context.h"

extern void run_script(script_context_t *ctx) noexcept;

int main() {
  script_context_t ctx;
  run_script(&ctx);
  printf("output %s\n",ctx.output_buffer.c_str());
  return 0;
}