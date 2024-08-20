// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/streams/streams.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/header.h"
#include "runtime-light/utils/context.h"

task_t<std::pair<char *, int32_t>> read_all_from_stream(uint64_t stream_d) {
  const auto &platform_ctx = *get_platform_context();
  constexpr int32_t batch_size = 32;

  int32_t buffer_capacity = batch_size;
  auto *buffer = static_cast<char *>(platform_ctx.allocator.alloc(buffer_capacity));
  int32_t buffer_size = 0;

  StreamStatus status{};
  do {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      co_return std::make_pair(nullptr, 0);
    }

    if (status.read_status == IOStatus::IOAvailable) {
      if (buffer_capacity - buffer_size < batch_size) {
        auto *new_buffer = static_cast<char *>(platform_ctx.allocator.alloc(static_cast<size_t>(buffer_capacity) * 2));
        std::memcpy(new_buffer, buffer, buffer_size);
        platform_ctx.allocator.free(buffer);
        buffer_capacity = buffer_capacity * 2;
        buffer = new_buffer;
      }
      buffer_size += platform_ctx.read(stream_d, batch_size, buffer + buffer_size);
    } else if (status.read_status == IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d};
    }

  } while (status.read_status != IOStatus::IOClosed);
  co_return std::make_pair(buffer, buffer_size);
}

std::pair<char *, int32_t> read_nonblock_from_stream(uint64_t stream_d) {
  const auto &platform_ctx = *get_platform_context();
  constexpr int32_t batch_size = 32;

  int32_t buffer_capacity = batch_size;
  auto *buffer = static_cast<char *>(platform_ctx.allocator.alloc(buffer_capacity));
  int32_t buffer_size = 0;

  StreamStatus status{};
  do {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      return std::make_pair(nullptr, 0);
    }

    if (status.read_status == IOStatus::IOAvailable) {
      if (buffer_capacity - buffer_size < batch_size) {
        auto *new_buffer = static_cast<char *>(platform_ctx.allocator.alloc(static_cast<size_t>(buffer_capacity) * 2));
        std::memcpy(new_buffer, buffer, buffer_size);
        platform_ctx.allocator.free(buffer);
        buffer_capacity = buffer_capacity * 2;
        buffer = new_buffer;
      }
      buffer_size += platform_ctx.read(stream_d, batch_size, buffer + buffer_size);
    } else {
      break;
    }
  } while (status.read_status != IOStatus::IOClosed);

  return std::make_pair(buffer, buffer_size);
}

task_t<int32_t> read_exact_from_stream(uint64_t stream_d, char *buffer, int32_t len) {
  const PlatformCtx &platform_ctx = *get_platform_context();

  int32_t read = 0;

  StreamStatus status{IOStatus::IOAvailable, IOStatus::IOAvailable, 0};
  while (read != len && status.read_status != IOStatus::IOClosed) {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      co_return 0;
    }

    if (status.read_status == IOStatus::IOAvailable) {
      read += platform_ctx.read(stream_d, len - read, buffer + read);
    } else if (status.read_status == IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d};
    } else {
      co_return read;
    }
  }

  co_return read;
}

task_t<int32_t> write_all_to_stream(uint64_t stream_d, const char *buffer, int32_t len) {
  const auto &platform_ctx = *get_platform_context();

  int32_t written = 0;

  StreamStatus status{};
  do {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      co_return written;
    }

    if (status.please_shutdown_write) {
      php_debug("stream %" PRIu64 " set please_shutdown_write. Stop writing", stream_d);
      co_return written;
    } else if (status.write_status == IOStatus::IOAvailable) {
      written += platform_ctx.write(stream_d, len - written, buffer + written);
    } else if (status.write_status == IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d};
    } else {
      php_warning("stream closed while writing. Wrote %d. Size %d. Stream %" PRIu64, written, len, stream_d);
      co_return written;
    }
  } while (written != len);

  php_debug("wrote %d bytes to stream %" PRIu64, len, stream_d);
  co_return written;
}

int32_t write_nonblock_to_stream(uint64_t stream_d, const char *buffer, int32_t len) {
  const auto &platform_ctx = *get_platform_context();

  int32_t written = 0;

  StreamStatus status{};
  do {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      return 0;
    }

    if (status.write_status == IOStatus::IOAvailable) {
      written += platform_ctx.write(stream_d, len - written, buffer + written);
    } else {
      break;
    }
  } while (written != len);

  php_debug("write %d bytes from %d to stream %" PRIu64, written, len, stream_d);
  return written;
}

task_t<int32_t> write_exact_to_stream(uint64_t stream_d, const char *buffer, int32_t len) {
  const auto &platform_ctx = *get_platform_context();

  int written = 0;

  StreamStatus status{IOAvailable, IOAvailable, 0};
  while (written != len && status.write_status != IOStatus::IOClosed) {
    GetStatusResult res = platform_ctx.get_stream_status(stream_d, std::addressof(status));
    if (res != GetStatusResult::GetStatusOk) {
      php_warning("get stream status returned status %d", res);
      co_return written;
    }

    if (status.please_shutdown_write) {
      php_debug("stream %" PRIu64 " set please_shutdown_write. Stop writing", stream_d);
      co_return written;
    } else if (status.write_status == IOStatus::IOAvailable) {
      written += platform_ctx.write(stream_d, len - written, buffer + written);
    } else if (status.write_status == IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d};
    } else {
      co_return written;
    }
  }

  co_return written;
}
