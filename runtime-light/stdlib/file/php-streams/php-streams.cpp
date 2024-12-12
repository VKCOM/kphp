// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/php-streams/php-streams.h"

#include "runtime-light/state/instance-state.h"

namespace resource_impl_ {
class_instance<ResourceWrapper> open_php_stream(const std::string_view scheme) noexcept {
  const std::string_view url{scheme.substr(PHP_STREAMS_PREFIX.size(), scheme.size() - PHP_STREAMS_PREFIX.size())};
  auto &file_stream_state{FileStreamInstanceState::get()};

  if (url == STDERR_NAME) {
    if (file_stream_state.stderr_wrapper.is_null()) {
      file_stream_state.stdout_wrapper.alloc(ResourceKind::Php, string(url.data()), INVALID_PLATFORM_DESCRIPTOR);
    }
    return file_stream_state.stderr_wrapper;
  } else if (url == STDOUT_NAME) {
    if (file_stream_state.stdout_wrapper.is_null()) {
      file_stream_state.stdout_wrapper.alloc(ResourceKind::Php, string(url.data()), InstanceState::get().standard_stream());
    }
    return file_stream_state.stdout_wrapper;
  } else if (url == STDIN_NAME) {
    if (file_stream_state.stdin_wrapper.is_null()) {
      file_stream_state.stdout_wrapper.alloc(ResourceKind::Php, string(url.data()), INVALID_PLATFORM_DESCRIPTOR);
    }
    return file_stream_state.stdin_wrapper;
  } else {
    php_warning("Unknown name %s for php stream", url.data());
    return class_instance<ResourceWrapper>{};
  }
}
} // namespace resource_impl_
