// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <csetjmp>

#include "context.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/logs.h"

inline void critical_error_handler() {
  constexpr const char *message = "script panic";
  const PlatformCtx &ptx = *get_platform_context();
  ComponentState &ctx = *get_component_context();
  ptx.log(Debug, strlen(message), message);

  if (ctx.not_finished()) {
    ctx.poll_status = PollStatus::PollFinishedError;
  }
  ptx.abort();
  exit(1);
}
