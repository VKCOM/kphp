// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-functions.h"

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/utils/logs.h"

resource f$fopen(const string& filename, [[maybe_unused]] const string& mode, [[maybe_unused]] bool use_include_path,
                 [[maybe_unused]] const resource& context) noexcept {
  kphp::fs::underlying_resource rsrc{{filename.c_str(), filename.size()}};
  if (rsrc.error_code() != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("fopen failed: {}", filename.c_str());
    return {};
  }

  return f$to_mixed(make_instance<kphp::fs::underlying_resource>(std::move(rsrc)));
}

resource f$stream_socket_client(const string& address, mixed& error_code, [[maybe_unused]] mixed& error_message, [[maybe_unused]] double timeout,
                                [[maybe_unused]] int64_t flags, [[maybe_unused]] const resource& context) noexcept {
  /*
   * TODO: Here should be waiting with timeout,
   *       but it can't be expressed simple ways by awaitables since we blocked inside k2
   * */
  const std::string_view address_view{address.c_str(), address.size()};
  const auto address_kind{kphp::fs::uri_to_resource_kind(address_view)};
  if (address_kind != kphp::fs::resource_kind::UDP) [[unlikely]] {
    return static_cast<int64_t>(k2::errno_einval);
  }

  kphp::fs::underlying_resource rsrc{address_view};
  if (rsrc.error_code() != k2::errno_ok) [[unlikely]] {
    error_code = static_cast<int64_t>(rsrc.error_code());
    return {};
  }
  return f$to_mixed(make_instance<kphp::fs::underlying_resource>(std::move(rsrc)));
}

Optional<string> f$file_get_contents(const string& stream) noexcept {
  kphp::fs::underlying_resource rsrc{{stream.c_str(), stream.size()}};
  if (rsrc.error_code() != k2::errno_ok) [[unlikely]] {
    return false;
  }
  return rsrc.get_contents();
}

kphp::coro::task<Optional<int64_t>> f$fwrite(resource stream, string text) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::fs::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fwrite: {}", stream.to_string().c_str());
    co_return false;
  }
  co_return co_await rsrc.get()->write({text.c_str(), text.size()});
}

bool f$fflush(const resource& stream) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::fs::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fflush: {}", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->flush();
  return true;
}

bool f$fclose(const resource& stream) noexcept {
  auto rsrc{from_mixed<class_instance<kphp::fs::underlying_resource>>(stream, {})};
  if (rsrc.is_null()) [[unlikely]] {
    kphp::log::warning("unexpected resource in fclose: {}", stream.to_string().c_str());
    return false;
  }

  rsrc.get()->close();
  return true;
}
