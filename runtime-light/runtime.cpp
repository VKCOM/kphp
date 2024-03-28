#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

extern task_t<void> (*k_main)(void);

const ImageInfo *vk_k2_describe() {
  static ImageInfo imageInfo {"echo", 1, 1, {1, 2, 3}};
  return &imageInfo;
}

ImageState *vk_k2_create_image_state(const Allocator *alloc) {
  return nullptr;
}

ComponentState *vk_k2_create_component_state(const ImageState *image_state, const Allocator *alloc) {
  (void)image_state;
  platformAllocator = alloc;
  // todo sjljmp
  char *buffer = static_cast<char *>(platformAllocator->alloc(sizeof(ComponentState)));
  componentState = new (buffer) ComponentState();
  // todo initial suspend
  componentState->k_main = k_main();
  return componentState;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  platformCtx = pt_ctx;
  platformAllocator = &pt_ctx->allocator;
  componentState = component_ctx;

  if (sigsetjmp(componentState->panic_buffer, 0) == 0) {
    uint64_t stream_d = 0;
    while (platformCtx->take_update(&stream_d)) {
      if (componentState->component_status == ComponentInited) {
        componentState->component_status = ComponentRunning;
        componentState->standard_stream = stream_d;
        componentState->k_main();
      } else if (componentState->awaited_stream == stream_d) {
        componentState->suspend_point();
      } else {
        // platform poll compomnent with unawaited stream_d
      }
    }
  } else {
    componentState->poll_status = PollStatus::PollFinished;
  }

  return componentState->poll_status;
}