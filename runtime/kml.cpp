#include "runtime-common/stdlib/kml/kml-inference-context.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"

namespace {
KmlInferenceContext kml_inference_context;
KmlModelsContext kml_models_context;
} // namespace

KmlInferenceContext& KmlInferenceContext::get() noexcept {
  return kml_inference_context;
}

KmlModelsContext& KmlModelsContext::get() noexcept {
  return kml_models_context;
}