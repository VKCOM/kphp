#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

task_t<void> k_main(void) noexcept;

ImageState *vk_k2_create_image_state(const Allocator *alloc) {
  // todo:k2 allocate memory for const value
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
  // todo:k2 allocate memory for mutable globals
  // coroutine is initial suspend
  componentState->k_main = k_main();
  componentState->standard_handle = componentState->k_main.get_handle();
  return componentState;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  platformCtx = pt_ctx;
  platformAllocator = &pt_ctx->allocator;
  componentState = component_ctx;

  if (sigsetjmp(componentState->exit_tag, 0) == 0) {
    if (componentState->poll_status == PollStatus::PollReschedule) {
      // If component was suspended by please yield and there is no awaitable streams
      componentState->standard_handle();
    }
    uint64_t stream_d = 0;
    while (platformCtx->take_update(&stream_d) && componentState->poll_status != PollStatus::PollFinished) {
      StreamStatus status;
      GetStatusResult res = platformCtx->get_stream_status(stream_d, &status);
      if (res != GetStatusOk) {
        break;
      }
      php_debug("opened_streams size %d", componentState->opened_streams.size());
      if (componentState->opened_streams.contains(stream_d)) {
        php_debug("update on processed stream %lu", stream_d);
        auto expected_status = componentState->opened_streams[stream_d];
        if ((expected_status == WBlocked && status.write_status != IOBlocked) ||
            (expected_status == RBlocked && status.read_status != IOBlocked)) {
          php_debug("resume on waited query %lu", stream_d);
          auto suspend_point = componentState->awaiting_coroutines[stream_d];
          componentState->awaiting_coroutines.erase(stream_d);
          php_assert(componentState->awaiting_coroutines.empty());
          suspend_point();
        }
      } else {
        bool already_pending = std::find(componentState->incoming_pending_queries.begin(), componentState->incoming_pending_queries.end(), stream_d)
                               != componentState->incoming_pending_queries.end();
        if (!already_pending) {
          php_debug("got new pending query %lu", stream_d);
          componentState->incoming_pending_queries.push_back(stream_d);
        }
        if (componentState->standard_stream == 0) {
          php_debug("start process pending query %lu", stream_d);
          componentState->standard_handle();
        }
      }
    }
  } else {
    componentState->poll_status = PollStatus::PollFinished;
  }

  return componentState->poll_status;
}