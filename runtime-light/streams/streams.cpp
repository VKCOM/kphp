// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/streams/streams.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/logs.h"

namespace {

constexpr int32_t STREAM_BATCH_SIZE = 4096;

} // namespace

kphp::coro::task<std::pair<std::unique_ptr<char, decltype(std::addressof(kphp::memory::script::free))>, size_t>>
read_all_from_stream(uint64_t stream_d) noexcept {
  using byte_pointer_t = std::unique_ptr<char, decltype(std::addressof(kphp::memory::script::free))>;

  int32_t buffer_capacity{STREAM_BATCH_SIZE};
  auto* buffer{static_cast<char*>(kphp::memory::script::alloc(buffer_capacity))};
  int32_t buffer_size{};

  k2::StreamStatus status{};
  do {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      co_return std::make_pair(byte_pointer_t{nullptr, kphp::memory::script::free}, 0);
    }

    if (status.read_status == k2::IOStatus::IOAvailable) {
      if (buffer_capacity - buffer_size < STREAM_BATCH_SIZE) {
        buffer_capacity = buffer_capacity * 2;
        buffer = static_cast<char*>(kphp::memory::script::realloc(static_cast<void*>(buffer), buffer_capacity));
      }
      buffer_size += k2::read(stream_d, STREAM_BATCH_SIZE, buffer + buffer_size);
    } else if (status.read_status == k2::IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d, wait_for_update_t::update_kind::read};
    }

  } while (status.read_status != k2::IOStatus::IOClosed);

  co_return std::make_pair(byte_pointer_t{buffer, kphp::memory::script::free}, buffer_size);
}

std::pair<std::unique_ptr<char, decltype(std::addressof(kphp::memory::script::free))>, int64_t> read_nonblock_from_stream(uint64_t stream_d) noexcept {
  using byte_pointer_t = std::unique_ptr<char, decltype(std::addressof(kphp::memory::script::free))>;

  int32_t buffer_capacity{STREAM_BATCH_SIZE};
  auto* buffer{static_cast<char*>(kphp::memory::script::alloc(buffer_capacity))};
  int32_t buffer_size{};

  k2::StreamStatus status{};
  do {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      return std::make_pair(byte_pointer_t{nullptr, kphp::memory::script::free}, 0);
    }

    if (status.read_status == k2::IOStatus::IOAvailable) {
      if (buffer_capacity - buffer_size < STREAM_BATCH_SIZE) {
        buffer_capacity = buffer_capacity * 2;
        buffer = static_cast<char*>(kphp::memory::script::realloc(static_cast<void*>(buffer), buffer_capacity));
      }
      buffer_size += k2::read(stream_d, STREAM_BATCH_SIZE, buffer + buffer_size);
    } else {
      break;
    }
  } while (status.read_status != k2::IOStatus::IOClosed);

  return std::make_pair(byte_pointer_t{buffer, kphp::memory::script::free}, buffer_size);
}

kphp::coro::task<int32_t> read_exact_from_stream(uint64_t stream_d, char* buffer, int32_t len) noexcept {
  int32_t read = 0;

  k2::StreamStatus status{
      .read_status = k2::IOStatus::IOAvailable, .write_status = k2::IOStatus::IOAvailable, .please_shutdown_write = 0, .libc_errno = k2::errno_ok};
  while (read != len && status.read_status != k2::IOStatus::IOClosed) {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      co_return 0;
    }

    if (status.read_status == k2::IOStatus::IOAvailable) {
      read += k2::read(stream_d, len - read, buffer + read);
    } else if (status.read_status == k2::IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d, wait_for_update_t::update_kind::read};
    } else {
      co_return read;
    }
  }

  co_return read;
}

kphp::coro::task<int32_t> write_all_to_stream(uint64_t stream_d, const char* buffer, int32_t len) noexcept {
  int32_t written = 0;

  k2::StreamStatus status{};
  do {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      co_return written;
    }

    if (status.please_shutdown_write) {
      kphp::log::debug("stream {} set please_shutdown_write. Stop writing", stream_d);
      co_return written;
    } else if (status.write_status == k2::IOStatus::IOAvailable) {
      written += k2::write(stream_d, len - written, buffer + written);
    } else if (status.write_status == k2::IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d, wait_for_update_t::update_kind::write};
    } else {
      kphp::log::warning("stream closed while writing. Wrote {}. Size {}. Stream {}", written, len, stream_d);
      co_return written;
    }
  } while (written != len);

  kphp::log::debug("wrote {} bytes to stream {}", len, stream_d);
  co_return written;
}

int32_t write_nonblock_to_stream(uint64_t stream_d, const char* buffer, int32_t len) noexcept {
  int32_t written = 0;

  k2::StreamStatus status{};
  do {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      return 0;
    }

    if (status.write_status == k2::IOStatus::IOAvailable) {
      written += k2::write(stream_d, len - written, buffer + written);
    } else {
      break;
    }
  } while (written != len);

  kphp::log::debug("write {} bytes from {} to stream {}", written, len, stream_d);
  return written;
}

kphp::coro::task<int32_t> write_exact_to_stream(uint64_t stream_d, const char* buffer, int32_t len) noexcept {
  int written = 0;

  k2::StreamStatus status{
      .read_status = k2::IOStatus::IOAvailable, .write_status = k2::IOStatus::IOAvailable, .please_shutdown_write = 0, .libc_errno = k2::errno_ok};
  while (written != len && status.write_status != k2::IOStatus::IOClosed) {
    k2::stream_status(stream_d, std::addressof(status));
    if (status.libc_errno != k2::errno_ok) {
      kphp::log::warning("get stream status returned status {}", status.libc_errno);
      co_return written;
    }

    if (status.please_shutdown_write) {
      kphp::log::debug("stream {} set please_shutdown_write. Stop writing", stream_d);
      co_return written;
    } else if (status.write_status == k2::IOStatus::IOAvailable) {
      written += k2::write(stream_d, len - written, buffer + written);
    } else if (status.write_status == k2::IOStatus::IOBlocked) {
      co_await wait_for_update_t{stream_d, wait_for_update_t::update_kind::write};
    } else {
      co_return written;
    }
  }

  co_return written;
}
