// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/curl/curl-state.h"
#include "runtime-light/state/instance-state.h"

CurlInstanceState& CurlInstanceState::get() noexcept {
  return InstanceState::get().curl_instance_state;
}
