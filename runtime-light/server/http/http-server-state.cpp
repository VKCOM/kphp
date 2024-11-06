// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/http-server-state.h"

#include "runtime-light/state/instance-state.h"

HttpServerInstanceState &HttpServerInstanceState::get() noexcept {
  return InstanceState::get().http_server_instance_state;
}
