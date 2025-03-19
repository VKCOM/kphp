// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/server/args-functions.h"
#include "runtime-light/state/instance-state.h"

Optional<array<mixed>> f$getopt([[maybe_unused]] const string & options, [[maybe_unused]] const array<string> & longopts,
                                       [[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> rest_index) noexcept {
  if (const auto &instance_st{InstanceState::get()}; instance_st.image_kind() != ImageKind::CLI) {
    return false;
  }

  return ComponentState::get().cli_opts;
}
