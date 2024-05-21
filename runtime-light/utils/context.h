#pragma once

#include "runtime-light/header.h"

extern thread_local ImageState * mutableImageState;
extern const thread_local ImageState * imageState;
extern const thread_local PlatformCtx * platformCtx;
extern thread_local ComponentState * componentState;

inline const PlatformCtx * get_platform_context() {
  return platformCtx;
}

inline ComponentState * get_component_context() {
  return componentState;
}

inline const ImageState * get_image_state() {
  return imageState;
}

inline ImageState * get_mutable_image_state() {
  return mutableImageState;
}

void reset_thread_locals();
