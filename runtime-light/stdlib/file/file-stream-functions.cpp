// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-stream-functions.h"

#include <tuple>

#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/file/php-streams/php-streams.h"
#include "runtime-light/stdlib/file/udp-streams/udp-streams.h"

namespace {

ResourceKind resolve_kind(const std::string_view &scheme) noexcept {
  if (scheme.starts_with(resource_impl_::UDP_RESOURCE_PREFIX)) {
    return ResourceKind::UdpResource;
  } else if (scheme.starts_with(resource_impl_::PHP_STREAMS_PREFIX)) {
    return ResourceKind::PhpResource;
  } else {
    return ResourceKind::Unknown;
  }
}

bool is_scheme(const std::string_view &filename) noexcept {
  std::string_view::size_type delimiter{filename.find("://")};
  return delimiter != std::string_view::npos && delimiter > 0 && filename.find("://", delimiter + 3 /* +3 to skip founded :// */) == std::string_view::npos;
}
} // namespace

resource f$fopen(const string &filename, [[maybe_unused]] const string &mode, [[maybe_unused]] bool use_include_path,
                 [[maybe_unused]] const resource &context) {
  const std::string_view scheme{filename.c_str(), filename.size()};
  if (!is_scheme(scheme)) {
    php_warning("Work with local files unsupported");
    return false;
  }

  ResourceKind kind{resolve_kind(scheme)};
  class_instance<ResourceWrapper> wrapper;
  switch (kind) {
    case ResourceKind::UdpResource:
      wrapper = resource_impl_::connect_to_host_by_udp(scheme).first;
      break;
    case ResourceKind::PhpResource:
      wrapper = resource_impl_::open_php_stream(scheme);
      break;
    default:
      php_warning("Unsupported scheme type %s", filename.c_str());
      return false;
  }
  if (wrapper.is_null()) {
    return false;
  }

  return f$to_mixed(wrapper);
}

resource f$stream_socket_client(const string &address, mixed &error_number, [[maybe_unused]] mixed &error_description, [[maybe_unused]] double timeout,
                                [[maybe_unused]] int64_t flags, [[maybe_unused]] const resource &context) noexcept {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  const std::string_view scheme{address.c_str(), address.size()};
  if (!is_scheme(scheme)) {
    php_warning("Wrong scheme in address %s. It should have pattern <kind>://<url>", scheme.data());
    return false;
  }

  ResourceKind kind{resolve_kind(scheme)};
  class_instance<ResourceWrapper> wrapper;
  int32_t error_code{k2::errno_ok};
  switch (kind) {
    case ResourceKind::UdpResource:
      std::tie(wrapper, error_code) = resource_impl_::connect_to_host_by_udp(scheme);
      break;
    default:
      php_warning("Cannot connect to %s", address.c_str());
      return false;
  }
  if (error_code != k2::errno_ok) {
    error_number = error_code;
    return false;
  }

  return f$to_mixed(wrapper);
}

task_t<Optional<int64_t>> f$fwrite(const resource &stream, const string &text) noexcept {
  if (!stream.is_object()) {
    php_warning("Try access unknown resource value %s", stream.to_string().c_str());
    co_return false;
  }

  auto typed_resource = from_mixed<class_instance<ResourceWrapper>>(stream, string(""));
  if (typed_resource.is_null()) {
    php_warning("try to fwrite in wrong resource %s", stream.to_string().c_str());
    co_return false;
  }

  const std::string_view text_view{text.c_str(), text.size()};
  co_return co_await typed_resource.get()->write(text_view);
}

task_t<Optional<string>> f$file_get_contents(const string &stream) noexcept {
  const std::string_view scheme{stream.c_str(), stream.size()};
  if (!is_scheme(scheme)) {
    php_warning("Work with local files unsupported");
    co_return false;
  }

  ResourceKind kind{resolve_kind(scheme)};
  class_instance<ResourceWrapper> wrapper;
  switch (kind) {
    case ResourceKind::PhpResource:
      wrapper = resource_impl_::open_php_stream(scheme);
      break;
    default:
      php_warning("Cannot perform file_get_contents on stream %s", scheme.data());
      co_return false;
  }

  if (wrapper.is_null()) {
    co_return false;
  }

  co_return co_await wrapper.get()->get_contents();
}

bool f$fflush(const resource &stream) noexcept {
  if (!stream.is_object()) {
    php_warning("Try access unknown resource value %s", stream.to_string().c_str());
    return false;
  }

  auto typed_resource = from_mixed<class_instance<ResourceWrapper>>(stream, string(""));
  if (typed_resource.is_null()) {
    php_warning("try to fwrite in wrong resource %s", stream.to_string().c_str());
    return false;
  }

  typed_resource.get()->flush();
  return true;
}

bool f$fclose(const resource &stream) noexcept {
  if (!stream.is_object()) {
    php_warning("Try access unknown resource value %s", stream.to_string().c_str());
    return false;
  }

  auto typed_resource = from_mixed<class_instance<ResourceWrapper>>(stream, string(""));
  if (typed_resource.is_null()) {
    php_warning("try to fwrite in wrong resource %s", stream.to_string().c_str());
    return false;
  }

  typed_resource.get()->close();
  return true;
}
