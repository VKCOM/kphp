// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-stream.h"

#include "runtime-light/component/component.h"

FileStreamComponentState &FileStreamComponentState::get() noexcept {
  return get_component_context()->file_stream_component_state;
}