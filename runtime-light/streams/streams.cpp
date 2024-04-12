#include "runtime-light/streams/streams.h"

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/component/component.h"
#include "runtime-light/context.h"
#include "runtime-light/coroutine/awaitable.h"

task_t<int8_t> read_magic_from_stream(uint64_t stream_d) {
  co_await test_yield_t{};

  const PlatformCtx & ptx = *get_platform_context();
  int count = sizeof(int8_t);
  int8_t magic;
  StreamStatus status;
  int buffer_size = 0;

  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      co_return -1;
    }

    if (status.read_status == IOAvailable) {
      buffer_size += ptx.read(stream_d, count - buffer_size, reinterpret_cast<char *>(&magic) + buffer_size);
    } else if (status.read_status == IOBlocked) {
      co_await read_blocked_t{stream_d};
    } else if (status.read_status == IOClosed) {
      php_warning("cannot read magic from stream %lu because IOClosed", stream_d);
      co_return -1;
    }
  } while (buffer_size != count);

  co_return magic;
}

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

task_t<bool> write_magic_to_stream(uint64_t stream_d, int8_t magic) {
  co_await test_yield_t{};

  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  int len = sizeof(int8_t);

  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      co_return false;
    }

    if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, reinterpret_cast<char *>(&magic) + writed);
    } else if (status.write_status == IOBlocked) {
      co_await write_blocked_t{stream_d};
    } else {
      php_warning("stream closed while writing. Writed %d. Size %d. Stream %lu", writed, len, stream_d);
      co_return false;
    }
  } while (writed != len);

  co_return true;
}

task_t<bool> write_query_with_magic_to_stream(uint64_t stream_d, int8_t magic, const char * buffer, int len) {
  bool ok = co_await write_magic_to_stream(stream_d, magic);
  if (!ok) {
    co_return false;
  }
  ok = co_await write_all_to_stream(stream_d, buffer, len);
  if (!ok) {
    co_return false;
  }
  co_return true;
}

task_t<bool> write_all_to_stream(uint64_t stream_d, const char * buffer, int len) {
  co_await test_yield_t{};

  StreamStatus status;
  const PlatformCtx & ptx = *get_platform_context();
  int writed = 0;
  do {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      php_warning("get stream status return status %d", res);
      co_return false;
    }

    if (status.write_status == IOAvailable) {
      writed += ptx.write(stream_d, len - writed, buffer + writed);
    } else if (status.write_status == IOBlocked) {
      co_await write_blocked_t{stream_d};
    } else {
      php_warning("stream closed while writing. Writed %d. Size %d. Stream %lu", writed, len, stream_d);
      co_return false;
    }
  } while (writed != len);

  php_debug("write %d bytes to stream %lu", len, stream_d);
  co_return true;
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
