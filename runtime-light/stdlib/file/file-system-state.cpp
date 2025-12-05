// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-state.h"

#include "runtime-light/state/image-state.h"

const FileSystemImageState& FileSystemImageState::get() noexcept {
  return ImageState::get().file_system_image_state;
}
