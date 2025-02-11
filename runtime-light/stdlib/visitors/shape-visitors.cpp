// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/visitors/shape-visitors.h"

#include "runtime-light/state/image-state.h"

const ShapeKeyDemangle &ShapeKeyDemangle::get() noexcept {
  return ImageState::get().shape_key_demangler;
}

ShapeKeyDemangle &ShapeKeyDemangle::get_mutable() noexcept {
  return ImageState::get_mutable().shape_key_demangler;
}
