// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/http/http-server-context.h"

#include "runtime-light/component/component.h"

HttpServerInstanceState &HttpServerInstanceState::get() noexcept {
  return InstanceState::get().http_server_instance_state;
}
