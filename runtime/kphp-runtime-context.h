#pragma once

#include "kphp-core/kphp-core-context.h"
#include "kphp-core/kphp_core.h"

#include "common/smart_ptrs/singleton.h"

struct KphpRuntimeContext : KphpCoreContext {
  friend class vk::singleton<KphpRuntimeContext>;

  void init();
  void free();

  string_buffer static_SB;
  string_buffer static_SB_spare;

private:
  KphpRuntimeContext() = default;
};

