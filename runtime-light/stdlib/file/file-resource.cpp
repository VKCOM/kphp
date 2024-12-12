// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-resource.h"

#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/file/php-streams/php-streams.h"
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

Optional<string> ResourceWrapper::get_contents() const noexcept {
  if (kind == ResourceKind::Php) {
    const std::string_view url_view{url.c_str(), url.size()};
    if (url_view == resource_impl_::STDIN_NAME) {
      auto opt_raw_post_data{HttpServerInstanceState::get().opt_raw_post_data};
      if (opt_raw_post_data.has_value()) {
        return opt_raw_post_data.value();
      }
    }
  }
  return false;
}
