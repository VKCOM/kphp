// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"

struct ExceptionInstanceState final : vk::not_copyable {
  Throwable cur_exception;

  ExceptionInstanceState() noexcept = default;

  static ExceptionInstanceState &get() noexcept;
};
