#pragma once

#include "runtime-light/header.h"

inline thread_local ImageState * mutableImageState;
inline const thread_local ImageState * imageState;
inline const thread_local PlatformCtx * platformCtx;
inline thread_local ComponentState * componentState;

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
