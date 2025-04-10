// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-state.h"

#include "runtime-light/state/instance-state.h"

FileSystemInstanceState& FileSystemInstanceState::get() noexcept {
  return InstanceState::get().file_system_instance_state;
}
