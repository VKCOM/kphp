#include "runtime-common/stdlib/kml/kml-inference-context.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"
#include "runtime/critical_section.h"

namespace {
KmlInferenceContext kml_inference_context;
KmlModelsContext kml_models_context;
} // namespace

KmlInferenceContext& KmlInferenceContext::get() noexcept {
  return kml_inference_context;
}

const KmlModelsContext& KmlModelsContext::get() noexcept {
  return kml_models_context;
}

KmlModelsContext& KmlModelsContext::get_mutable() noexcept {
  return kml_models_context;
}

void enable_malloc_in_inference() {
  dl::enter_critical_section();
}

void disable_malloc_in_inference() {
  dl::leave_critical_section();
}
