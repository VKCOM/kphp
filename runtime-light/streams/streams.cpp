#include "runtime-light/streams/streams.h"

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/component/component.h"
#include "runtime-light/context.h"
#include "runtime-light/coroutine/awaitable.h"

task_t<std::pair<char *, int>> read_all_from_stream(uint64_t stream_d) {
  co_await test_yield_t{};

  constexpr int batch_size = 32;
  const PlatformCtx & ptx = *get_platform_context();
  int buffer_capacity = batch_size;
  char * buffer = static_cast<char *>(ptx.allocator.alloc(buffer_capacity));
  int buffer_size = 0;
  StreamStatus status;

  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      co_return std::make_pair(nullptr, 0);
    }

    if (status.read_status == IOAvailable) {
      if (buffer_capacity - buffer_size < batch_size) {
        char * new_buffer = static_cast<char *>(ptx.allocator.alloc(buffer_capacity * 2));
        memcpy(new_buffer, buffer, buffer_size);
        ptx.allocator.free(buffer);
        buffer_capacity = buffer_capacity * 2;
        buffer = new_buffer;
      }
      buffer_size += ptx.read(stream_d, batch_size, buffer + buffer_size);
    } else if (status.read_status == IOBlocked) {
      get_component_context()->awaited_stream = stream_d;
      co_await platform_switch_t{};
    }
  } while (status.read_status != IOClosed);

  co_return std::make_pair(buffer, buffer_size);
}

task_t<bool> write_all_to_stream(uint64_t stream_d, const char * buffer, int len) {
  co_await test_yield_t{};

  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      co_return false;
    }

    if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, buffer + writed);
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
