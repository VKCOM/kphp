#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

extern task_t<void> (*k_main)(void);

ImageState *vk_k2_create_image_state(const Allocator *alloc) {
  return nullptr;
}

ComponentState *vk_k2_create_component_state(const ImageState *image_state, const Allocator *alloc) {
  (void)image_state;
  platformAllocator = alloc;
  sigjmp_buf exit_tag;
  if (sigsetjmp(exit_tag, 0) == 0) {
    char *buffer = static_cast<char *>(platformAllocator->alloc(sizeof(ComponentState)));
    componentState = new (buffer) ComponentState();
  }
  // coroutine is initial suspend
  componentState->k_main = k_main();
  return componentState;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  platformCtx = pt_ctx;
  platformAllocator = &pt_ctx->allocator;
  componentState = component_ctx;

  if (sigsetjmp(componentState->exit_tag, 0) == 0) {
    if (componentState->poll_status == PollStatus::PollReschedule) {
      // If component was suspended by please yield and there is no awaitable streams
      componentState->suspend_point();
    } else {
      uint64_t stream_d = 0;
      while (platformCtx->take_update(&stream_d)) {
        if (componentState->component_status == ComponentInited) {
          // Component get incoming query
          componentState->component_status = ComponentRunning;
          componentState->standard_stream = stream_d;
          componentState->k_main();
        } else if (componentState->awaited_stream == stream_d) {
          // Component resume on awaited stream
          componentState->awaited_stream = -1;
          componentState->suspend_point();
        } else {
          // Component resume on an-awaited stream
          // assert(false);
        }
      }
    }

  } else {
    componentState->poll_status = PollStatus::PollFinished;
  }

  return componentState->poll_status;
}