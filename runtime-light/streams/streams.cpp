#include "runtime-light/streams/streams.h"

#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/context.h"

std::pair<char *, int> read_all_from_stream(uint64_t stream_d) {

  return {nullptr, 0};
}

task_t<bool> write_all_to_stream(uint64_t stream_d, const char * buffer, int len) {
  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      co_return false;
    }

    if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, buffer);
    } else if (status.write_status == IOBlocked) {
      get_component_context()->awaited_stream = stream_d;
      co_await platform_switch_t{};
    } else {
      co_return false;
    }
  } while (writed != len);

  ptx.shutdown_write(stream_d);
  co_return true;
}
