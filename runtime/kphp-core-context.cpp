#include "common/smart_ptrs/singleton.h"
#include "runtime/kphp-runtime-context.h"
#include "server/php-engine-vars.h"

KphpCoreContext &KphpCoreContext::current() noexcept {
  return kphpRuntimeContext;
}

void KphpCoreContext::init() {
  if (static_buffer_length_limit < 0) {
    init_string_buffer_lib(266175, (1 << 24));
  } else {
    init_string_buffer_lib(266175, static_buffer_length_limit);
  }
}

void KphpCoreContext::free() {
  free_migration_php8();
}
