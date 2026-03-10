// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/contextual-tags.h"

#include <cstddef>
#include <functional>
#include <optional>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

namespace kphp::log {

std::optional<std::reference_wrapper<contextual_tags>> contextual_tags::try_get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_tags;
  }
  return std::nullopt;
}

} // namespace kphp::log
