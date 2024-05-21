#pragma once

#include "kphp-core/kphp-core-context.h"

#include "common/smart_ptrs/singleton.h"

struct KphpRuntimeContext : KphpCoreContext {
  void init();
  void free();

  friend class vk::singleton<KphpRuntimeContext>;
};

