// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/utils/context.h"

thread_local ImageState *mutableImageState;
const thread_local ImageState *imageState;
const thread_local PlatformCtx *platformCtx;
thread_local InstanceState *componentState;

void reset_thread_locals() {
  mutableImageState = nullptr;
  imageState = nullptr;
  platformCtx = nullptr;
  componentState = nullptr;
}
