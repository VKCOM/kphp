// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-resource.h"

#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"

task_t<int64_t> ResourceWrapper::write(const std::string_view text) const noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    co_return 0;
  }
  co_return co_await write_all_to_stream(stream_d, text.data(), text.size());
}

void ResourceWrapper::flush() noexcept {}

void ResourceWrapper::close() noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    return;
  }

  switch (kind) {
    case ResourceKind::Php:
      /*
       * PHP support multiple opening/closing operations on standard IO streams.
       * */
      stream_d = INVALID_PLATFORM_DESCRIPTOR;
      break;
    case ResourceKind::Udp:
      InstanceState::get().release_stream(stream_d);
      stream_d = INVALID_PLATFORM_DESCRIPTOR;
      break;
    case ResourceKind::Unknown:
      break;
  }
}

task_t<Optional<string>> ResourceWrapper::get_contents() const noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    co_return false;
  }

  if (kind == ResourceKind::Php) {
    auto [buffer, size]{co_await read_all_from_stream(stream_d)};
    string result{buffer, static_cast<string::size_type>(size)};
    k2::free(buffer);
    co_return result;
  } else {
    co_return false;
  }
}
