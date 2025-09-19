#include "runtime-light/stdlib/kml/kml-instance-state.h"
#include "runtime-light/state/instance-state.h"

KmlInstanceState& KmlInstanceState::get() noexcept {
  return InstanceState::get().kml_instance_state;
}

KmlInferenceContext& KmlInferenceContext::get() noexcept {
  return KmlInstanceState::get().kml_inference_context;
}

void enable_malloc_in_inference() {
  InstanceState::get().instance_allocator_state.enable_libc_alloc();
}

void disable_malloc_in_inference() {
  InstanceState::get().instance_allocator_state.disable_libc_alloc();
}