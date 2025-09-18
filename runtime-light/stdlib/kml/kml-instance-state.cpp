#include "runtime-light/stdlib/kml/kml-instance-state.h"
#include "runtime-light/state/instance-state.h"

KmlInstanceState& KmlInstanceState::get() noexcept {
  return InstanceState::get().kml_instance_state;
}

KmlInferenceContext& KmlInferenceContext::get() noexcept {
  return KmlInstanceState::get().kml_inference_context;
}