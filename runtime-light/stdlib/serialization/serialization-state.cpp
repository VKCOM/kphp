// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/serialization/serialization-state.h"

#include "runtime-light/state/instance-state.h"

SerializationInstanceState& SerializationInstanceState::get() noexcept {
  return InstanceState::get().serialization_instance_state;
}
