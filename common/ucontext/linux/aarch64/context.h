#include <ucontext.h>

inline constexpr void *get_context_stack_ptr_portable(const ucontext_t &ctx) noexcept {
  return ctx.uc_stack.ss_sp;
}

inline constexpr size_t get_context_stack_size_portable(const ucontext_t &ctx) noexcept {
  return ctx.uc_stack.ss_size;
}

inline constexpr void set_context_stack_ptr_portable(ucontext_t &ctx, void *sp) noexcept {
  ctx.uc_stack.ss_sp = sp;
}

inline constexpr void set_context_stack_size_portable(ucontext_t &ctx, size_t size) noexcept {
  ctx.uc_stack.ss_size = size;
}

inline constexpr void set_context_link_portable(ucontext_t &ctx, ucontext_t *link) noexcept {
  ctx.uc_link = link;
}

inline void *get_context_stack_base_ptr_portable(const ucontext_t &ctx) noexcept {
  // 29 -- magic constant, based on libc implementation from `sysdeps/unix/sysv/linux/aarch64/ucontext-internal.h`
  return reinterpret_cast<void *>(ctx.uc_mcontext.regs[29]);
}