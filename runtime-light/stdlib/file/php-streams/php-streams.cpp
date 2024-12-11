// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/php-streams/php-streams.h"

#include <cstring>

#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/file/php-streams/php-streams-context.h"
#include "runtime-light/streams/streams.h"

namespace {

constexpr std::string_view STDERR_NAME = "stderr";
constexpr std::string_view STDOUT_NAME = "stdout";
constexpr std::string_view STDIN_NAME = "stdin";

} // namespace

namespace resource_impl_ {

task_t<int64_t> PhpResourceWrapper::write(const std::string_view text) noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    co_return 0;
  }
  co_return co_await write_all_to_stream(stream_d, text.data(), text.size());
}

task_t<Optional<string>> PhpResourceWrapper::get_contents() noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    co_return false;
  }
  auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer, static_cast<string::size_type>(size)};
  k2::free(buffer);
  co_return result;
}

class_instance<PhpResourceWrapper> open_php_stream(const std::string_view scheme) noexcept {
  const std::string_view url{scheme.substr(PHP_STREAMS_PREFIX.size(), scheme.size() - PHP_STREAMS_PREFIX.size())};
  auto &php_stream_state{PhpStreamInstanceState::get()};

  if (url == STDERR_NAME) {
    if (php_stream_state.stderr_wrapper.is_null()) {
      php_stream_state.stderr_wrapper.alloc();
      php_stream_state.stderr_wrapper.get()->stream_d = INVALID_PLATFORM_DESCRIPTOR;
    }
    return php_stream_state.stderr_wrapper;
  } else if (url == STDOUT_NAME) {
    if (php_stream_state.stdout_wrapper.is_null()) {
      php_stream_state.stdout_wrapper.alloc();
      php_stream_state.stdout_wrapper.get()->stream_d = InstanceState::get().standard_stream();
    }
    return php_stream_state.stdout_wrapper;
  } else if (url == STDIN_NAME) {
    if (php_stream_state.stdin_wrapper.is_null()) {
      php_stream_state.stdin_wrapper.alloc();
      php_stream_state.stdin_wrapper.get()->stream_d = InstanceState::get().standard_stream();
    }
    return php_stream_state.stdin_wrapper;
  } else {
    php_warning("Unknown name %s for php stream", url.data());
    return class_instance<PhpResourceWrapper>{};
  }
}
} // namespace resource_impl_
