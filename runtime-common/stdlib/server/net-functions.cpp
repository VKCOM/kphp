// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/server/net-functions.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "runtime-common/core/runtime-core.h"

namespace {
constexpr int64_t IPV4_SIZE = 25;
}

Optional<string> f$inet_pton(const string& address) noexcept {
  int32_t address_family{};
  int32_t size{};

  if (strchr(address.c_str(), ':')) {
    address_family = AF_INET6;
    size = 16;
  } else if (strchr(address.c_str(), '.')) {
    address_family = AF_INET;
    size = 4;
  } else {
    php_warning("Unrecognized address \"%s\"", address.c_str());
    return false;
  }

  string result{static_cast<string::size_type>(size), false};
  if (inet_pton(address_family, address.c_str(), result.buffer()) <= 0) {
    php_warning("Unrecognized address \"%s\"", address.c_str());
    return false;
  }

  return result;
}

Optional<int64_t> f$ip2long(const string& ip) noexcept {
  struct in_addr result;
  if (inet_pton(AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }
  return ntohl(result.s_addr);
}

Optional<string> f$ip2ulong(const string& ip) noexcept {
  struct in_addr result;
  if (inet_pton(AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }

  char buf[IPV4_SIZE];
  int len = snprintf(buf, IPV4_SIZE, "%u", ntohl(result.s_addr));
  return string(buf, len);
}

string f$long2ip(int64_t num) noexcept {
  auto& runtime_context{RuntimeContext::get()};
  runtime_context.static_SB.clean().reserve(IPV4_SIZE);
  for (int i = 3; i >= 0; i--) {
    runtime_context.static_SB << ((num >> (i * 8)) & 255);
    if (i) {
      runtime_context.static_SB.append_char('.');
    }
  }
  return runtime_context.static_SB.str();
}
