#include "common/smart_ptrs/singleton.h"
#include "kphp-runtime-context.h"

KphpCoreContext &KphpCoreContext::current() noexcept {
  return vk::singleton<KphpRuntimeContext>::get();
}
