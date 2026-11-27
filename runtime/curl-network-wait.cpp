// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cerrno>
#include <netdb.h>
#include <poll.h>
#include <sys/select.h>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

namespace {
template<class F>
int network_wait(bool waiting, F&& call) {
  auto* timer = BuiltinTimeNetworkScope::current();
  const int before_errno = errno;
  const auto started = waiting && timer ? BuiltinTimeClock::now() : BuiltinTimeClock::time_point{};
  errno = before_errno;
  const int result = call();
  const int after_errno = errno;
  if (waiting && timer) {
    timer->subtract_elapsed_ns(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(BuiltinTimeClock::now() - started).count()));
  }
  errno = after_errno;
  return result;
}
} // namespace

// References are redirected in the bundled libcurl archive only. These
// forwarding functions preserve timeout, EINTR and error behavior of master.
extern "C" int kphp_curl_poll(pollfd* fds, nfds_t count, int timeout) {
  return network_wait(timeout != 0, [&] { return poll(fds, count, timeout); });
}

extern "C" int kphp_curl_select(int count, fd_set* read, fd_set* write, fd_set* error, timeval* timeout) {
  const bool waiting = timeout == nullptr || timeout->tv_sec != 0 || timeout->tv_usec != 0;
  return network_wait(waiting, [&] { return select(count, read, write, error, timeout); });
}

extern "C" int kphp_curl_getaddrinfo(const char* host, const char* service, const addrinfo* hints, addrinfo** result) {
  // libc/NSS hides resolver network waits. Exclude the same complete resolver
  // boundary on KPHP and K2, including the CPU work inside this one function.
  return network_wait(true, [&] { return getaddrinfo(host, service, hints, result); });
}
