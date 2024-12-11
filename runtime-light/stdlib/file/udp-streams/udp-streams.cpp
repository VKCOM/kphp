// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/udp-streams/udp-streams.h"

#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"

namespace resource_impl_ {
std::pair<class_instance<ResourceWrapper>, int32_t> open_udp_stream(const std::string_view scheme) noexcept {
  const std::string_view url{scheme.substr(UDP_RESOURCE_PREFIX.size(), scheme.size() - UDP_RESOURCE_PREFIX.size())};
  auto connection{InstanceState::get().open_stream(url, StreamKind::Udp)};
  class_instance<ResourceWrapper> wrapper;
  if (connection.stream_d != INVALID_PLATFORM_DESCRIPTOR) {
    wrapper.alloc(ResourceKind::Udp, connection.stream_d);
  }
  return {wrapper, connection.error_code};
}
}
