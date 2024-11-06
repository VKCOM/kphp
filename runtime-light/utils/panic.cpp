// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/utils/logs.h"

void critical_error_handler() {
  constexpr const char *message = "script panic";
  k2::log(LogLevel::Debug, std::char_traits<char>::length(message), message);

  if (k2::instance_state() != nullptr) {
    InstanceState::get().poll_status = k2::PollStatus::PollFinishedError;
  }
  k2::exit(1);
}
