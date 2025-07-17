// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/cli/cli-instance-state.h"

#include "runtime-light/state/instance-state.h"

CLIInstanceInstance& CLIInstanceInstance::get() noexcept {
  return InstanceState::get().cli_instance_instate;
}
