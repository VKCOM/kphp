// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-stream-functions.h"

#include <tuple>
#include <utility>

#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/file/php-streams/php-streams.h"
#include "runtime-light/stdlib/file/udp-streams/udp-streams.h"
#include "runtime-light/streams/streams.h"

namespace {

std::pair<ResourceType, uint64_t> resolve_stream_resource_type(const string &stream) noexcept {
  if (stream.empty()) {
    return {ResourceType::Unknown, 0};
  }

  const auto delimiter{stream.find_first_of(string(":"))};
  if (delimiter == string::npos) {
    return {ResourceType::Unknown, 0};
  }

  string resource_type{stream.substr(0, delimiter)};
  string stream_d{stream.substr(delimiter + 1, stream.size() - delimiter)};
  if (!resource_type.is_numeric() || !stream_d.is_numeric()) {
    return {ResourceType::Unknown, 0};
  }

  return {static_cast<ResourceType>(resource_type.to_int()), stream_d.to_int()};
}

std::pair<ResourceType, string> resolve_address_resource_type(const string &address) noexcept {
  const std::string_view address_view{address.c_str(), address.size()};
  if (address_view.starts_with(UDP_STREAMS_PREFIX)) {
    return {ResourceType::UdpStream, address.substr(UDP_STREAMS_PREFIX.size(), address.size() - UDP_STREAMS_PREFIX.size())};
  } else if (address_view.starts_with(PHP_STREAMS_PREFIX)) {
    return {ResourceType::PhpStream, address.substr(PHP_STREAMS_PREFIX.size(), address.size() - PHP_STREAMS_PREFIX.size())};
  } else {
    return {ResourceType::Unknown, string()};
  }
}

bool is_resource_url(const string &filename) noexcept {
  return filename.find(string("://")) != string::npos;
}
} // namespace

resource f$fopen(const string &filename, [[maybe_unused]] const string &mode, [[maybe_unused]] bool use_include_path,
                 [[maybe_unused]] const resource &context) {
  if (!is_resource_url(filename)) {
    php_warning("Work with local files unsupported");
    return false;
  }

  const auto [type, url]{resolve_address_resource_type(filename)};
  uint64_t stream_d{INVALID_PLATFORM_DESCRIPTOR};
  switch (type) {
    case ResourceType::UdpStream:
      stream_d = connect_to_host_by_udp(url).first;
      break;
    case ResourceType::PhpStream:
      stream_d = open_php_stream(url);
      break;
    default:
      php_warning("Unsupported scheme type %s", filename.c_str());
      return false;
  }
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    return false;
  }

  return {string(static_cast<int64_t>(type)).append(":").append(static_cast<int64_t>(stream_d))};
}

resource f$stream_socket_client(const string &address, mixed &error_number, [[maybe_unused]] mixed &error_description, [[maybe_unused]] double timeout,
                                [[maybe_unused]] int64_t flags, [[maybe_unused]] const resource &context) {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  const auto [type, url]{resolve_address_resource_type(address)};
  uint64_t stream_d{INVALID_PLATFORM_DESCRIPTOR};
  int32_t error_code{0};
  switch (type) {
    case ResourceType::UdpStream:
      std::tie(stream_d, error_code) = connect_to_host_by_udp(url);
      break;
    default:
      php_warning("Unable to connect to %s", address.c_str());
      return false;
  }
  error_number = error_code;
  if (error_code != 0) {
    return false;
  }

  return {string(static_cast<int64_t>(type)).append(":").append(static_cast<int64_t>(stream_d))};
}

task_t<Optional<string>> f$file_get_contents(const string &filename) {
  const resource stream = f$fopen(filename, string{"r"});
  if (!stream.to_bool()) {
    co_return false;
  }
  const auto [_, stream_d]{resolve_stream_resource_type(stream.to_string())};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer, static_cast<string::size_type>(size)};
  k2::free(buffer);
  co_return result;
}

task_t<Optional<int64_t>> f$fwrite(const resource &stream, const string &text) noexcept {
  auto [type, stream_d]{resolve_stream_resource_type(stream.to_string())};
  if (type == ResourceType::Unknown) {
    php_warning("try to fwrite in wrong resource %s", stream.to_string().c_str());
    co_return Optional<int64_t>{};
  }
  co_return co_await write_all_to_stream(stream_d, text.c_str(), text.size());
}

bool f$fclose(const resource &stream) noexcept {
  auto [type, stream_d]{resolve_stream_resource_type(stream.to_string())};
  if (type == ResourceType::Unknown) {
    php_warning("try to fclose wrong resource %s", stream.to_string().c_str());
    return false;
  }
  if (InstanceState::get().standard_stream() == stream_d) {
    /*
     * PHP support multiple opening/closing operations on standard IO streams.
     * Because of this, execution of fclose on standard_stream which can be obtained by urls php://
     * shouldn't close it in k2.
     *
     * TODO: maybe better way to not give standard_stream directly in open_php_stream,
     *       but now it's done that way for ease of implementation
     * */
    return true;
  }
  InstanceState::get().release_stream(stream_d);
  return true;
}
