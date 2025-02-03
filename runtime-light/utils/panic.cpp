// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string_view>
#include <utility>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/utils/logs.h"

void critical_error_handler() {
  constexpr std::string_view message = "script panic";
  k2::log(std::to_underlying(LogLevel::Debug), message.size(), message.data());

  if (k2::instance_state() != nullptr) {
    InstanceState::get().poll_status = k2::PollStatus::PollFinishedError;
  }
  k2::exit(1);
}
