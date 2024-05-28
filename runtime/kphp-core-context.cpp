#include "common/smart_ptrs/singleton.h"
#include "runtime/kphp-runtime-context.h"

KphpCoreContext &KphpCoreContext::current() noexcept {
  return kphpRuntimeContext;
}
