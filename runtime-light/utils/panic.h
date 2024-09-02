// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <csetjmp>

#include "context.h"
#include "runtime-light/component/component.h"
#include "runtime-light/header.h"
#include "runtime-light/utils/logs.h"

inline void critical_error_handler() {
  constexpr const char *message = "script panic";
  const auto &platform_ctx = *get_platform_context();
  auto &component_ctx = *get_component_context();
  platform_ctx.log(Debug, strlen(message), message);

  if (component_ctx.poll_status != PollStatus::PollFinishedOk && component_ctx.poll_status != PollStatus::PollFinishedError) {
    component_ctx.poll_status = PollStatus::PollFinishedError;
  }
  platform_ctx.abort();
  exit(1);
}
