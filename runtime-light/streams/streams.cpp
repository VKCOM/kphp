#include "runtime-light/streams/streams.h"

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/component/component.h"
#include "runtime-light/context.h"
#include "runtime-light/coroutine/awaitable.h"

task_t<string> read_all_from_stream(uint64_t stream_d) {
  constexpr int batch_size = 16;
  const PlatformCtx & ptx = *get_platform_context();
  int buffer_capacity = batch_size;
  char * buffer = static_cast<char *>(dl::allocate(buffer_capacity));
  int buffer_size = 0;
  StreamStatus status;

  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      co_return string();
    }

    if (status.read_status == IOAvailable) {
      if (buffer_capacity - buffer_size < batch_size) {
        buffer = static_cast<char *>(dl::reallocate(buffer, buffer_capacity * 1.5, buffer_capacity));
        buffer_capacity = buffer_capacity * 1.5;
      }
      buffer_size += ptx.read(stream_d, batch_size, buffer + buffer_size);
    } else if (status.read_status == IOBlocked) {
      get_component_context()->awaited_stream = stream_d;
      co_await platform_switch_t{};
    }
  } while (status.read_status != IOClosed);
  string result(buffer, buffer_size);
  dl::deallocate(buffer, buffer_capacity);
  co_return result;
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
