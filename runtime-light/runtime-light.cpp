#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"
#include "runtime-light/core/globals/php-init-scripts.h"

ImageState *vk_k2_create_image_state(const struct PlatformCtx *pt_ctx) {
  // Note that in vk_k2_create_image_state available only allocator and logs from pt_ctx
  platformCtx = pt_ctx;
  php_debug("create image state on \"%s\"", vk_k2_describe()->image_name);
  char *buffer = static_cast<char *>(platformCtx->allocator.alloc(sizeof(ImageState)));
  if (buffer == nullptr) {
    php_warning("cannot allocate enough memory for ImageState");
    return nullptr;
  }
  mutableImageState = new (buffer) ImageState();
  imageState = mutableImageState;
  init_php_scripts_once_in_master();
  ImageState * mutable_image_state = mutableImageState;
  php_debug("finish image state creation on \"%s\"", vk_k2_describe()->image_name);
  reset_thread_locals();
  return mutable_image_state;
}

ComponentState *vk_k2_create_component_state(const struct ImageState *image_state, const struct PlatformCtx *pt_ctx) {
  // Note that in vk_k2_create_component_state available only allocator and logs from pt_ctx
  imageState = image_state;
  platformCtx = pt_ctx;
  php_debug("create component state on \"%s\"", vk_k2_describe()->image_name);
  sigjmp_buf exit_tag;
  if (sigsetjmp(exit_tag, 0) == 0) {
    char *buffer = static_cast<char *>(platformCtx->allocator.alloc(sizeof(ComponentState)));
    if (buffer == nullptr) {
      php_warning("cannot allocate enough memory for ComponentState");
      return nullptr;
    }
    componentState = new (buffer) ComponentState();
  } else {
    php_warning("force jump while ComponentState initialization");
    return nullptr;
  }
  // coroutine is initial suspend
  init_php_scripts_in_each_worker(componentState->php_script_mutable_globals_singleton, componentState->k_main);
  componentState->standard_handle = componentState->k_main.get_handle();
  ComponentState * component_state = componentState;
  php_debug("finish component state creation on \"%s\"", vk_k2_describe()->image_name);
  reset_thread_locals();
  return component_state;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, ComponentState *component_ctx) {
  imageState = image_state;
  platformCtx = pt_ctx;
  componentState = component_ctx;

  if (sigsetjmp(componentState->exit_tag, 0) == 0) {
    if (componentState->poll_status == PollStatus::PollReschedule) {
      // If component was suspended by please yield and there is no awaitable streams
      componentState->standard_handle();
    }
    uint64_t stream_d = 0;
    while (platformCtx->take_update(&stream_d) && componentState->not_finished()) {
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
    componentState->poll_status = PollStatus::PollFinishedError;
  }
  PollStatus poll_status = componentState->poll_status;
  reset_thread_locals();
  return poll_status;
}
