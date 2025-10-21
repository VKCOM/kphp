// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/streams/stream.h"

using resource = mixed;

namespace kphp::fs {

inline constexpr std::string_view SCHEME_DELIMITER = "://";

inline constexpr std::string_view PHP_SCHEME_PREFIX = "php://";
inline constexpr std::string_view STDIN_NAME = "php://stdin";
inline constexpr std::string_view STDOUT_NAME = "php://stdout";
inline constexpr std::string_view STDERR_NAME = "php://stderr";
inline constexpr std::string_view INPUT_NAME = "php://input";

inline constexpr std::string_view TCP_SCHEME_PREFIX = "tcp://";
inline constexpr std::string_view UDP_SCHEME_PREFIX = "udp://";

// ================================================================================================

struct resource : public refcountable_polymorphic_php_classes<may_be_mixed_base> {
  const char* get_class() const noexcept final {
    return "resource";
  }
};

// ================================================================================================

struct sync_resource : public resource {
  virtual auto write(std::span<const std::byte>) noexcept -> std::expected<size_t, int32_t> = 0;
  virtual auto read(std::span<std::byte>) noexcept -> std::expected<size_t, int32_t> = 0;
  virtual auto pread(std::span<std::byte>, off_t) noexcept -> std::expected<size_t, int32_t> = 0;
  virtual auto get_contents() noexcept -> std::expected<string, int32_t> = 0;
  virtual auto flush() noexcept -> std::expected<void, int32_t> = 0;
  virtual auto close() noexcept -> std::expected<void, int32_t> = 0;
  virtual auto eof() const noexcept -> std::expected<bool, int32_t> = 0;
};

// ================================================================================================

class file : public sync_resource {
  bool m_eof{};
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};

  explicit file(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {}

public:
  file(file&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)) {}

  file& operator=(file&& other) noexcept {
    if (this != std::addressof(other)) {
      std::ignore = close();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
    }
    return *this;
  }

  ~file() override {
    std::ignore = close();
  }

  file(const file&) = delete;
  file& operator=(const file&) = delete;

  static auto open(std::string_view path, std::string_view mode) noexcept -> std::expected<file, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;
};

inline auto file::open(std::string_view path, std::string_view mode) noexcept -> std::expected<file, int32_t> {
  k2::descriptor descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  if (auto error_code{k2::fopen(std::addressof(descriptor), path, mode)}; error_code != k2::errno_ok) [[unlikely]] {
    return std::unexpected{error_code};
  }
  return {file{descriptor}};
}

inline auto file::write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {k2::write(m_descriptor, buf.size(), buf.data())};
}

inline auto file::read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }

  const auto read{k2::read(m_descriptor, buf.size(), buf.data())};
  if (buf.size() != 0) [[likely]] {
    m_eof = read == 0;
  }

  return read;
}

inline auto file::pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }

  const auto read{k2::pread(m_descriptor, buf.size(), buf.data(), offset)};
  // we do not change `m_eof`, because pread is readonly operation

  return read;
}

inline auto file::get_contents() noexcept -> std::expected<string, int32_t> {
  return std::unexpected{m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR ? k2::errno_efault : k2::errno_enodev};
}

inline auto file::flush() noexcept -> std::expected<void, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {};
}

inline auto file::close() noexcept -> std::expected<void, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  return {};
}

inline auto file::eof() const noexcept -> std::expected<bool, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return m_eof;
}

// ================================================================================================

class directory : public sync_resource {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};

  explicit directory(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {}

public:
  directory(directory&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)) {}
  directory& operator=(directory&& other) noexcept {
    if (this != std::addressof(other)) {
      std::ignore = close();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
    }
    return *this;
  }

  ~directory() override {
    std::ignore = close();
  }

  directory(const directory&) = delete;
  directory& operator=(const directory&) = delete;

  static auto open(std::string_view path) noexcept -> std::expected<directory, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;

  auto readdir() const noexcept -> std::invoke_result_t<decltype(std::addressof(k2::readdir)), k2::descriptor>;
};

inline auto directory::open(std::string_view path) noexcept -> std::expected<directory, int32_t> {
  auto expected{k2::opendir(path)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  return directory{*expected};
}

inline auto directory::write(std::span<const std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::read(std::span<std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::pread(std::span<std::byte> /*buf*/, off_t /*offset*/) noexcept -> std::expected<size_t, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::get_contents() noexcept -> std::expected<string, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::flush() noexcept -> std::expected<void, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::close() noexcept -> std::expected<void, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  return {};
}

inline auto directory::eof() const noexcept -> std::expected<bool, int32_t> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::unexpected{k2::errno_einval};
}

inline auto directory::readdir() const noexcept -> std::invoke_result_t<decltype(std::addressof(k2::readdir)), k2::descriptor> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return k2::readdir(m_descriptor);
}

// ================================================================================================

class stdinput : public sync_resource {
public:
  static auto open() noexcept -> std::expected<stdinput, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;
};

inline auto stdinput::open() noexcept -> std::expected<stdinput, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::write(std::span<const std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::read(std::span<std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::pread(std::span<std::byte> /*buf*/, off_t /*offset*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::get_contents() noexcept -> std::expected<string, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::flush() noexcept -> std::expected<void, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::close() noexcept -> std::expected<void, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

inline auto stdinput::eof() const noexcept -> std::expected<bool, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

// ================================================================================================

class stdoutput : public sync_resource {
  bool m_open{true};

public:
  static auto open() noexcept -> std::expected<stdoutput, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;
};

inline auto stdoutput::write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }

  auto& current_output_buf{OutputInstanceState::get().output_buffers.current_buffer().get()};
  current_output_buf.append(reinterpret_cast<const char*>(buf.data()), buf.size());
  return std::expected<size_t, int32_t>{buf.size()};
}

inline auto stdoutput::read(std::span<std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stdoutput::pread(std::span<std::byte> /*buf*/, off_t /*offset*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stdoutput::get_contents() noexcept -> std::expected<string, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stdoutput::flush() noexcept -> std::expected<void, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::expected<void, int32_t>{};
}

inline auto stdoutput::close() noexcept -> std::expected<void, int32_t> {
  if (!std::exchange(m_open, false)) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::expected<void, int32_t>{};
}

inline auto stdoutput::eof() const noexcept -> std::expected<bool, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

// ================================================================================================

class stderror : public sync_resource {
  bool m_open{true};
  // Later, we want to have a specific descriptor for stderr and use k2 stream api
  // For now, we have k2_stderr_write function for writing

public:
  static auto open() noexcept -> std::expected<stderror, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;
};

inline auto stderror::open() noexcept -> std::expected<stderror, int32_t> {
  return {stderror{}};
}

inline auto stderror::write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {k2::stderr_write(buf.size(), buf.data())};
}

inline auto stderror::read(std::span<std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stderror::pread(std::span<std::byte> /*buf*/, off_t /*offset*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stderror::get_contents() noexcept -> std::expected<string, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto stderror::flush() noexcept -> std::expected<void, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {};
}

inline auto stderror::close() noexcept -> std::expected<void, int32_t> {
  if (!std::exchange(m_open, false)) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {};
}

inline auto stderror::eof() const noexcept -> std::expected<bool, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

// ================================================================================================

class input : public sync_resource {
  bool m_open{true};

public:
  static auto open() noexcept -> std::expected<input, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> override;
  auto pread(std::span<std::byte> buf, off_t offset) noexcept -> std::expected<size_t, int32_t> override;
  auto get_contents() noexcept -> std::expected<string, int32_t> override;
  auto flush() noexcept -> std::expected<void, int32_t> override;
  auto close() noexcept -> std::expected<void, int32_t> override;
  auto eof() const noexcept -> std::expected<bool, int32_t> override;
};

inline auto input::write(std::span<const std::byte> /*buf*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto input::read(std::span<std::byte> buf) noexcept -> std::expected<size_t, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }

  auto& http_server_instance_st{HttpServerInstanceState::get()};
  if (!http_server_instance_st.opt_raw_post_data) {
    return {0};
  }

  const auto& raw_post_data{*http_server_instance_st.opt_raw_post_data};
  const auto size{std::min(buf.size(), static_cast<size_t>(raw_post_data.size()))};
  std::memcpy(buf.data(), reinterpret_cast<const std::byte*>(raw_post_data.c_str()), size);
  return {size};
}

inline auto input::pread(std::span<std::byte> /*buf*/, off_t /*offset*/) noexcept -> std::expected<size_t, int32_t> {
  return std::unexpected{m_open ? k2::errno_einval : k2::errno_enodev};
}

inline auto input::get_contents() noexcept -> std::expected<string, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return std::expected<string, int32_t>{HttpServerInstanceState::get().opt_raw_post_data.value_or(string{})};
}

inline auto input::flush() noexcept -> std::expected<void, int32_t> {
  if (!m_open) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {};
}

inline auto input::close() noexcept -> std::expected<void, int32_t> {
  if (!std::exchange(m_open, false)) [[unlikely]] {
    return std::unexpected{k2::errno_enodev};
  }
  return {};
}

inline auto input::eof() const noexcept -> std::expected<bool, int32_t> {
  return std::unexpected{k2::errno_enodev};
}

// ================================================================================================

struct async_resource : public resource {
  virtual auto write(std::span<const std::byte>) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> = 0;
  virtual auto read(std::span<std::byte>) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> = 0;
  virtual auto get_contents() noexcept -> kphp::coro::task<std::expected<string, int32_t>> = 0;
  virtual auto flush() noexcept -> kphp::coro::task<std::expected<void, int32_t>> = 0;
  virtual auto close() noexcept -> kphp::coro::task<std::expected<void, int32_t>> = 0;
};

// ================================================================================================

// FIXME: Address Buffered Socket Data Loss Issue
//
// Problem Description:
// This class implements buffered I/O for socket operations. A potential issue arises
// when a user creates a socket and stores it in a global variable. If the user writes
// data to the socket using `fwrite` but does not explicitly call `fflush`, the buffered
// data may never be sent. This occurs because destructors for global variables are not
// guaranteed to be called before program termination, leading to unsent data remaining
// in the buffer.
class socket : public async_resource {
  bool m_open{true};
  kphp::component::stream m_stream;
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> m_buf;

  explicit socket(kphp::component::stream stream) noexcept
      : m_stream(std::move(stream)) {}

  auto finalized() const noexcept -> bool;
  static auto finalizer(kphp::component::stream stream, kphp::stl::vector<std::byte, kphp::memory::script_allocator> buf) noexcept -> kphp::coro::task<>;

public:
  socket(socket&& other) noexcept
      : m_open(std::exchange(other.m_open, false)),
        m_stream(std::move(other.m_stream)),
        m_buf(std::move(other.m_buf)) {}

  socket& operator=(socket&& other) noexcept {
    if (this != std::addressof(other)) {
      if (!finalized()) {
        kphp::coro::io_scheduler::get().spawn(finalizer(std::move(m_stream), std::move(m_buf)));
      }

      m_open = std::exchange(other.m_open, false);
      m_stream = std::move(other.m_stream);
      m_buf = std::move(other.m_buf);
    }
    return *this;
  }

  ~socket() override {
    if (!finalized()) {
      kphp::coro::io_scheduler::get().spawn(finalizer(std::move(m_stream), std::move(m_buf)));
    }
  }

  socket(const socket&) = delete;
  socket& operator=(const socket&) = delete;

  static auto open(std::string_view scheme) noexcept -> std::expected<socket, int32_t>;

  auto write(std::span<const std::byte> buf) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> override;
  auto read(std::span<std::byte> buf) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> override;
  auto get_contents() noexcept -> kphp::coro::task<std::expected<string, int32_t>> override;
  auto flush() noexcept -> kphp::coro::task<std::expected<void, int32_t>> override;
  auto close() noexcept -> kphp::coro::task<std::expected<void, int32_t>> override;
};

inline auto socket::finalized() const noexcept -> bool {
  return !m_open || m_buf.empty();
}

inline auto socket::finalizer(kphp::component::stream stream, kphp::stl::vector<std::byte, kphp::memory::script_allocator> buf) noexcept -> kphp::coro::task<> {
  std::ignore = co_await stream.write_all({buf});
}

inline auto socket::open(std::string_view scheme) noexcept -> std::expected<socket, int32_t> {
  std::expected<socket, int32_t> expected{std::unexpected{k2::errno_einval}};
  if (scheme.starts_with(UDP_SCHEME_PREFIX)) {
    const auto hostname = scheme.substr(UDP_SCHEME_PREFIX.size(), scheme.size() - UDP_SCHEME_PREFIX.size());
    expected = kphp::component::stream::open(hostname, k2::stream_kind::udp).transform([](kphp::component::stream stream) noexcept {
      return socket{std::move(stream)};
    });
  } else if (scheme.starts_with(TCP_SCHEME_PREFIX)) {
    const auto hostname = scheme.substr(TCP_SCHEME_PREFIX.size(), scheme.size() - TCP_SCHEME_PREFIX.size());
    expected = kphp::component::stream::open(hostname, k2::stream_kind::tcp).transform([](kphp::component::stream stream) noexcept {
      return socket{std::move(stream)};
    });
  }
  return expected;
}

inline auto socket::write(std::span<const std::byte> buf) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  if (!m_open) [[unlikely]] {
    co_return std::unexpected{k2::errno_enodev};
  }
  m_buf.append_range(buf);
  co_return std::expected<size_t, int32_t>{buf.size()};
}

inline auto socket::read(std::span<std::byte> buf) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  if (!m_open) [[unlikely]] {
    co_return std::unexpected{k2::errno_enodev};
  }
  co_return co_await m_stream.read(buf);
}

inline auto socket::get_contents() noexcept -> kphp::coro::task<std::expected<string, int32_t>> {
  co_return std::unexpected{m_open ? k2::errno_efault : k2::errno_enodev};
}

inline auto socket::flush() noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (!m_open) [[unlikely]] {
    co_return std::unexpected{k2::errno_enodev};
  }

  if (auto expected{co_await m_stream.write_all({m_buf})}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  m_buf.clear();
  co_return std::expected<void, int32_t>{};
}

inline auto socket::close() noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await flush()}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }

  m_open = false;
  m_stream.reset(k2::INVALID_PLATFORM_DESCRIPTOR);
  co_return std::expected<void, int32_t>{};
}

} // namespace kphp::fs
