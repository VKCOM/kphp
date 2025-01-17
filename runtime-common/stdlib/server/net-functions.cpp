// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/server/net-functions.h"

#include <arpa/inet.h>
#include <cstdio>

Optional<int64_t> f$ip2long(const string &ip) noexcept {
  struct in_addr result;
  if (inet_pton(AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }
  return ntohl(result.s_addr);
}

Optional<string> f$ip2ulong(const string &ip) noexcept {
  struct in_addr result;
  if (inet_pton(AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }

  const size_t buf_size = 25;
  char buf[buf_size];
  int len = snprintf(buf, buf_size, "%u", ntohl(result.s_addr));
  return string(buf, len);
}

string f$long2ip(int64_t num) noexcept {
  auto &runtime_context{RuntimeContext::get()};
  runtime_context.static_SB.clean().reserve(20);
  for (int i = 3; i >= 0; i--) {
    runtime_context.static_SB << ((num >> (i * 8)) & 255);
    if (i) {
      runtime_context.static_SB.append_char('.');
    }
  }
  return runtime_context.static_SB.str();
}
