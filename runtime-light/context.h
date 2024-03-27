#pragma once

#include "runtime-light/scheme.h"

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
