// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-functions.h"

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/file/resource.h"

resource f$fopen(const string &filename, [[maybe_unused]] const string &mode, [[maybe_unused]] bool use_include_path,
                 [[maybe_unused]] const resource &context) noexcept {
  underlying_resource_t rsrc{{filename.c_str(), filename.size()}};
  if (rsrc.last_errc != k2::errno_ok) [[unlikely]] {
    return {};
  }

  return f$to_mixed(make_instance<underlying_resource_t>(std::move(rsrc)));
}

resource f$stream_socket_client(const string &address, mixed &error_code, [[maybe_unused]] mixed &error_message, [[maybe_unused]] double timeout,
                                [[maybe_unused]] int64_t flags, [[maybe_unused]] const resource &context) noexcept {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  const std::string_view address_view{address.c_str(), address.size()};
  const auto address_kind{resource_impl_::uri_to_resource_kind(address_view)};
  if (address_kind != resource_kind::UDP) [[unlikely]] {
    return static_cast<int64_t>(k2::errno_einval);
  }

  underlying_resource_t rsrc{address_view};
  if (rsrc.last_errc != k2::errno_ok) [[unlikely]] {
    error_code = static_cast<int64_t>(rsrc.last_errc);
    return {};
  }
  return f$to_mixed(make_instance<underlying_resource_t>(std::move(rsrc)));
}

Optional<string> f$file_get_contents(const string &stream) noexcept {
  underlying_resource_t rsrc{{stream.c_str(), stream.size()}};
  if (rsrc.last_errc != k2::errno_ok) [[unlikely]] {
    return false;
  }
  return rsrc.get_contents();
}

task_t<Optional<int64_t>> f$fwrite(const resource &stream, string text) noexcept {
  auto rsrc{from_mixed<class_instance<underlying_resource_t>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    php_warning("wrong resource in fwrite %s", stream.to_string().c_str());
    co_return false;
  }

  co_return co_await rsrc.get()->write({text.c_str(), text.size()});
}

bool f$fflush(const resource &stream) noexcept {
  auto rsrc{from_mixed<class_instance<underlying_resource_t>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    php_warning("wrong resource in fflush %s", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->flush();
  return true;
}

bool f$fclose(const resource &stream) noexcept {
  auto rsrc{from_mixed<class_instance<underlying_resource_t>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    php_warning("wrong resource in fclose: %s", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->close();
  return true;
}
