#include "runtime-light/stdlib/kml/kml-component-state.h"
#include "runtime-light/state/component-state.h"

const KmlComponentState& KmlComponentState::get() noexcept {
  return ComponentState::get().kml_component_state;
}

KmlComponentState& KmlComponentState::get_mutable() noexcept {
  return ComponentState::get_mutable().kml_component_state;
}

const KmlModelsContext& KmlModelsContext::get() noexcept {
  return KmlComponentState::get().kml_models_context;
}
KmlModelsContext& KmlModelsContext::get_mutable() noexcept {
  return KmlComponentState::get_mutable().kml_models_context;
}
