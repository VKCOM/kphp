// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/kml/kml-state.h"

#include "runtime-light/state/component-state.h"
#include "runtime-light/state/instance-state.h"

template<>
const KmlComponentState& KmlComponentState::get() noexcept {
  return ComponentState::get().kml_component_state;
}

template<>
KmlComponentState& KmlComponentState::get_mutable() noexcept {
  return const_cast<KmlComponentState&>(KmlComponentState::get());
}

template<>
KmlInstanceState& KmlInstanceState::get() noexcept {
  return InstanceState::get().kml_instance_state;
}
