// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/udp-streams/udp-streams.h"

#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"

namespace resource_impl_ {

task_t<int64_t> UdpResourceWrapper::write(const std::string_view text) noexcept {
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    co_return 0;
  }
  co_return co_await write_all_to_stream(stream_d, text.data(), text.size());
}

void UdpResourceWrapper::close() noexcept {
  InstanceState::get().release_stream(stream_d);
  stream_d = INVALID_PLATFORM_DESCRIPTOR;
}

std::pair<class_instance<UdpResourceWrapper>, int32_t> connect_to_host_by_udp(const std::string_view scheme) noexcept {
  const std::string_view url{scheme.substr(UDP_RESOURCE_PREFIX.size(), scheme.size() - UDP_RESOURCE_PREFIX.size())};
  auto connection{InstanceState::get().open_stream(url, StreamKind::Udp)};
  class_instance<UdpResourceWrapper> wrapper;
  if (connection.stream_d != INVALID_PLATFORM_DESCRIPTOR) {
    wrapper.alloc();
    wrapper.get()->stream_d = connection.stream_d;
  }
  return {wrapper, connection.error_code};
}
}
