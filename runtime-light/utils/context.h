#pragma once

#include "runtime-light/header.h"

inline thread_local ImageState * mutableImageState;
inline const thread_local ImageState * imageState;
inline const thread_local PlatformCtx * platformCtx;
inline const thread_local Allocator * platformAllocator;
inline thread_local ComponentState * componentState;

inline const PlatformCtx * get_platform_context() {
  return platformCtx;
}

inline const Allocator * get_platform_allocator() {
  return platformAllocator;
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
