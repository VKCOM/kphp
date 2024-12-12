// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/underlying-resource.h"

#include <string_view>
#include <tuple>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

underlying_resource_t::underlying_resource_t(std::string_view uri) noexcept {
  auto &instance_st{InstanceState::get()};
  if (uri == resource_impl_::STDIN_NAME) {
    stream_d_ = INVALID_PLATFORM_DESCRIPTOR;
    kind = kind_t::in;
  } else if (uri == resource_impl_::STDOUT_NAME) {
    stream_d_ = instance_st.standard_stream();
    kind = kind_t::out;
  } else if (uri == resource_impl_::STDERR_NAME) {
    stream_d_ = INVALID_PLATFORM_DESCRIPTOR;
    kind = kind_t::err;
  } else if (uri.starts_with(resource_impl_::UDP_SCHEME_PREFIX)) {
    const auto url{uri.substr(resource_impl_::UDP_SCHEME_PREFIX.size(), uri.size() - resource_impl_::UDP_SCHEME_PREFIX.size())};
    std::tie(stream_d_, last_error) = instance_st.open_stream(url, k2::StreamKind::UDP);
    kind = last_error == k2::errno_ok ? kind_t::udp : kind_t::unknown;
  } else {
    stream_d_ = INVALID_PLATFORM_DESCRIPTOR;
    last_error = k2::errno_einval;
    kind = kind_t::unknown;
  }
}

underlying_resource_t::underlying_resource_t(underlying_resource_t &&other) noexcept
  : stream_d_(std::exchange(other.stream_d_, INVALID_PLATFORM_DESCRIPTOR))
  , kind(std::exchange(other.kind, kind_t::unknown))
  , last_error(std::exchange(other.last_error, k2::errno_ok)) {}

underlying_resource_t::~underlying_resource_t() {
  if (stream_d_ == INVALID_PLATFORM_DESCRIPTOR) {
    return;
  }
  close();
}

void underlying_resource_t::close() noexcept {
  if (stream_d_ == INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    return;
  }

  switch (kind) {
    case kind_t::in:
    case kind_t::out:
    case kind_t::err: {
      // PHP supports multiple opening/closing operations on standard IO streams.
      stream_d_ = INVALID_PLATFORM_DESCRIPTOR;
      break;
    }
    case kind_t::udp: {
      InstanceState::get().release_stream(stream_d_);
      stream_d_ = INVALID_PLATFORM_DESCRIPTOR;
      break;
    }
    case kind_t::unknown: {
      break;
    }
  }
}
