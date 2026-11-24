// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/visitors/shape-key-demangle.h"
#include "runtime-light/components/kphp/state/image-state.h"

const ShapeKeyDemangle& ShapeKeyDemangle::get() noexcept {
  return ImageState::get().shape_key_demangler;
}
