// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/udp-streams/udp-streams.h"

#include "runtime-light/state/instance-state.h"

std::pair<uint64_t, int32_t> connect_to_host_by_udp(const string &hostname) noexcept {
  return InstanceState::get().connect_to(hostname, false);
}
