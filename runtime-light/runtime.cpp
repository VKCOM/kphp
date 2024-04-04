#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

task_t<void> k_main(void) noexcept;

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
  componentState->suspend_point = componentState->k_main.get_handle();
  return componentState;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  platformCtx = pt_ctx;
  platformAllocator = &pt_ctx->allocator;
  componentState = component_ctx;

  if (sigsetjmp(componentState->exit_tag, 0) == 0) {
    uint64_t stream_d = 0;
    do {
      if (componentState->poll_status == PollStatus::PollReschedule) {
        // If component was suspended by please yield and there is no awaitable streams
        componentState->suspend_point();
      } else if (componentState->awaited_stream == stream_d) {
        // Component resume on awaited stream
        componentState->awaited_stream = 0;
        componentState->suspend_point();
      } else if (stream_d != 0) {
        // Component get new query
        componentState->pending_queries.push(stream_d);
        if (componentState->standard_stream == 0) {
          componentState->suspend_point();
        }
      }
    } while (platformCtx->take_update(&stream_d));
  } else {
    componentState->poll_status = PollStatus::PollFinished;
  }

  return componentState->poll_status;
}