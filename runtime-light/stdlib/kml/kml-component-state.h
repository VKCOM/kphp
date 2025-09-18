#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"

struct KmlComponentState final : private vk::not_copyable {
  KmlModelsContext kml_models_context;

  KmlComponentState() noexcept = default;
  static const KmlComponentState& get() noexcept;
  static KmlComponentState& get_mutable() noexcept;
};
