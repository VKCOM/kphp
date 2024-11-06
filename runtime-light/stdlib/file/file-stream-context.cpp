// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-stream-context.h"

#include "runtime-light/state/instance-state.h"

FileStreamInstanceState &FileStreamInstanceState::get() noexcept {
  return InstanceState::get().file_stream_instance_state;
}
