#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"
#include "runtime-light/core/globals/php-init-scripts.h"

ImageState *vk_k2_create_image_state(const Allocator *alloc) {
  platformAllocator = alloc;
  char *buffer = static_cast<char *>(platformAllocator->alloc(sizeof(ImageState)));
  if (buffer == nullptr) {
    return nullptr;
  }
  mutableImageState = new (buffer) ImageState();
  imageState = mutableImageState;
  init_php_scripts_once_in_master(alloc);
  ImageState * mutable_image_state = mutableImageState;
  reset_thread_locals();
  return mutable_image_state;
}

ComponentState *vk_k2_create_component_state(const ImageState *image_state, const Allocator *alloc) {
  imageState = image_state;
  platformAllocator = alloc;
  sigjmp_buf exit_tag;
  if (sigsetjmp(exit_tag, 0) == 0) {
    char *buffer = static_cast<char *>(platformAllocator->alloc(sizeof(ComponentState)));
    if (buffer == nullptr) {
      return nullptr;
    }
    componentState = new (buffer) ComponentState();
  } else {
    return nullptr;
  }
  // coroutine is initial suspend
  init_php_scripts_in_each_worker(componentState->php_script_mutable_globals_singleton, componentState->k_main);
  componentState->standard_handle = componentState->k_main.get_handle();
  ComponentState * component_state = componentState;
  reset_thread_locals();
  return component_state;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  imageState = image_state;
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
      php_debug("opened_streams size %zu", componentState->opened_streams.size());
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
  PollStatus poll_status = componentState->poll_status;
  reset_thread_locals();
  return poll_status;
}
