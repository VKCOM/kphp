// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/resource.h"

#include <string_view>
#include <tuple>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

underlying_resource_t::underlying_resource_t(std::string_view scheme) noexcept
    : kind(resource_impl_::uri_to_resource_kind(scheme)) {
  auto& instance_st{InstanceState::get()};
  switch (kind) {
  case resource_kind::STDIN: {
    last_errc = k2::errno_einval;
    break;
  }
  case resource_kind::STDERR: {
    // Later, we want to have a specific descriptor for stderr and use k2 stream api
    // For now, we have k2_stderr_write function for writing
    last_errc = k2::errno_ok;
    break;
  }
  case resource_kind::STDOUT: {
    if (instance_st.image_kind() == image_kind::cli) {
      // stream_d_ = instance_st.standard_stream(); TODO
    } else {
      last_errc = k2::errno_einval;
    }
    break;
  }
  case resource_kind::INPUT: {
    last_errc = instance_st.image_kind() == image_kind::server ? k2::errno_ok : k2::errno_einval;
    break;
  }
  case resource_kind::UDP: {
    // const auto url{scheme.substr(resource_impl_::UDP_SCHEME_PREFIX.size(), scheme.size() - resource_impl_::UDP_SCHEME_PREFIX.size())}; TODO
    // std::tie(stream_d_, last_errc) = instance_st.open_stream(url, k2::stream_kind::udp);
    kind = last_errc == k2::errno_ok ? resource_kind::UDP : resource_kind::UNKNOWN;
    break;
  }
  case resource_kind::UNKNOWN: {
    last_errc = k2::errno_einval;
    break;
  }
  }
}

underlying_resource_t::underlying_resource_t(underlying_resource_t&& other) noexcept
    : m_opt_stream(std::move(other.m_opt_stream)),
      kind(std::exchange(other.kind, resource_kind::UNKNOWN)),
      last_errc(std::exchange(other.last_errc, k2::errno_ok)) {}

underlying_resource_t::~underlying_resource_t() {
  close();
}

void underlying_resource_t::close() noexcept { // TODO
  // if (stream_d_ == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
  //   return;
  // }
  //
  // switch (kind) {
  // case resource_kind::STDIN:
  //   [[fallthrough]];
  // case resource_kind::STDOUT:
  //   [[fallthrough]];
  // case resource_kind::STDERR:
  //   [[fallthrough]];
  // case resource_kind::INPUT: {
  //   // PHP supports multiple opening/closing operations on standard IO streams.
  //   stream_d_ = k2::INVALID_PLATFORM_DESCRIPTOR;
  //   break;
  // }
  // case resource_kind::UDP: {
  //   InstanceState::get().release_stream(stream_d_);
  //   stream_d_ = k2::INVALID_PLATFORM_DESCRIPTOR;
  //   break;
  // }
  // case resource_kind::UNKNOWN: {
  //   break;
  // }
  // }
}
