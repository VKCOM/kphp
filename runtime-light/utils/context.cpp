#include "runtime-light/utils/context.h"


void reset_thread_locals() {
  mutableImageState = nullptr;
  imageState = nullptr;
  platformCtx = nullptr;
  platformAllocator = nullptr;
  componentState = nullptr;
}