#include "runtime-light/utils/context.h"

thread_local ImageState * mutableImageState;
const thread_local ImageState * imageState;
const thread_local PlatformCtx * platformCtx;
thread_local ComponentState * componentState;

void reset_thread_locals() {
  mutableImageState = nullptr;
  imageState = nullptr;
  platformCtx = nullptr;
  componentState = nullptr;
}