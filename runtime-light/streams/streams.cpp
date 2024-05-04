#include "runtime-light/streams/streams.h"

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/utils/context.h"

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
      php_warning("get stream status return status %d", res);
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
      co_await read_blocked_t{stream_d};
    }
  } while (status.read_status != IOClosed);

  co_return std::make_pair(buffer, buffer_size);
}

task_t<int> write_all_to_stream(uint64_t stream_d, const char * buffer, int len) {
  co_await test_yield_t{};

  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      co_return writed;
    }

    if (status.please_shutdown_write) {
      php_notice("stream %d set please_shutdown_write. Stop writing", stream_d);
      co_return writed;
    } else if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, buffer + writed);
    } else if (status.write_status == IOBlocked) {
      co_await write_blocked_t{stream_d};
    } else {
      php_warning("stream closed while writing. Writed %d. Size %d. Stream %lu", writed, len, stream_d);
      co_return writed;
    }
  } while (writed != len);

  php_debug("write %d bytes to stream %lu", len, stream_d);
  co_return writed;
}

std::pair<char *, int> read_nonblock_from_stream(uint64_t stream_d) {
  constexpr int batch_size = 32;
  const PlatformCtx & ptx = *get_platform_context();
  int buffer_capacity = batch_size;
  char * buffer = static_cast<char *>(ptx.allocator.alloc(buffer_capacity));
  int buffer_size = 0;
  StreamStatus status;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      return std::make_pair(nullptr, 0);
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
    } else {
      break;
    }
  } while (status.read_status != IOClosed);

  return std::make_pair(buffer, buffer_size);
}

int write_nonblock_to_stream(uint64_t stream_d, const char * buffer, int len) {
  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      return 0;
    }

    if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, buffer + writed);
    } else {
      break;
    }
  } while (writed != len);

  php_debug("write %d bytes from %d to stream %lu", writed, len, stream_d);
  return writed;
}

void free_all_descriptors() {
  ComponentState & ctx = *get_component_context();
  const PlatformCtx & ptx = *get_platform_context();
  for (auto & processed_query : ctx.opened_streams) {
    ptx.free_descriptor(processed_query.first);
  }
  ctx.opened_streams.clear();
  ctx.awaiting_coroutines.clear();
  ptx.free_descriptor(ctx.standard_stream);
  ctx.standard_stream = 0;
}

void free_descriptor(uint64_t stream_d) {
  php_debug("free descriptor %lu", stream_d);
  ComponentState & ctx = *get_component_context();
  get_platform_context()->free_descriptor(stream_d);
  ctx.opened_streams.erase(stream_d);
  ctx.awaiting_coroutines.erase(stream_d);
}
