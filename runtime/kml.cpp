// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kml.h"

#include <cstdio>

#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/stdlib/kml/inference-context.h"
#include "runtime-common/stdlib/kml/models-context.h"

namespace kphp::kml {

const char* kml_dir = nullptr;

} // namespace kphp::kml

namespace {

KmlModelsState kml_models_context;
KmlInferenceState kml_inference_context;

} // namespace

template<>
const KmlModelsContext<kphp::kml::detail::dir_traverser, kphp::memory::platform_allocator>&
KmlModelsContext<kphp::kml::detail::dir_traverser, kphp::memory::platform_allocator>::get() noexcept {
  return kml_models_context;
}

template<>
KmlModelsContext<kphp::kml::detail::dir_traverser, kphp::memory::platform_allocator>&
KmlModelsContext<kphp::kml::detail::dir_traverser, kphp::memory::platform_allocator>::get_mutable() noexcept {
  return kml_models_context;
}

template<>
KmlInferenceContext<kphp::memory::platform_allocator>& KmlInferenceContext<kphp::memory::platform_allocator>::get() noexcept {
  return kml_inference_context;
}
