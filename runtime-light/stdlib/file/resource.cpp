// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/resource.h"

#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/stream.h"

namespace kphp::fs {

underlying_resource::underlying_resource(std::string_view scheme) noexcept {
  const auto kind{uri_to_resource_kind(scheme)};
  switch (kind) {
  case resource_kind::STDIN:
    m_errc = k2::errno_einval;
    break;
  case resource_kind::STDERR:
    // Later, we want to have a specific descriptor for stderr and use k2 stream api
    // For now, we have k2_stderr_write function for writing
    break;
  case resource_kind::STDOUT:
    m_errc = InstanceState::get().image_kind() == image_kind::cli ? k2::errno_ok : k2::errno_einval;
    break;
  case resource_kind::INPUT:
    m_errc = InstanceState::get().image_kind() == image_kind::server ? k2::errno_ok : k2::errno_einval;
    break;
  case resource_kind::UDP: {
    const auto url{scheme.substr(detail::UDP_SCHEME_PREFIX.size(), scheme.size() - detail::UDP_SCHEME_PREFIX.size())};
    auto expected{kphp::component::stream::open(url, k2::stream_kind::udp)};
    if (!expected) [[unlikely]] {
      m_errc = expected.error();
      break;
    }

    m_stream.emplace<kphp::component::stream>(std::move(*expected));
    break;
  }
  case resource_kind::UNKNOWN:
    m_errc = k2::errno_einval;
    break;
  }

  m_kind = m_errc == k2::errno_ok ? kind : resource_kind::UNKNOWN;
}

} // namespace kphp::fs
