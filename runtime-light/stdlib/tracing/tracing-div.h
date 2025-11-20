// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

struct C$KphpDiv : public refcountable_php_classes<C$KphpDiv>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  C$KphpDiv() noexcept = default;
};

inline int64_t f$KphpDiv$$assignTraceCtx([[maybe_unused]] const class_instance<C$KphpDiv>& v$this, [[maybe_unused]] int64_t int1, [[maybe_unused]] int64_t int2,
                                         [[maybe_unused]] const Optional<int64_t>& override_div_id) {

  return {};
}

inline std::tuple<int64_t, int64_t> f$KphpDiv$$generateTraceCtxForChild([[maybe_unused]] const class_instance<C$KphpDiv>& v$this,
                                                                        [[maybe_unused]] int64_t div_id, [[maybe_unused]] int64_t trace_flags) {
  return {};
}

inline double f$KphpDiv$$getStartTimestamp([[maybe_unused]] const class_instance<C$KphpDiv>& v$this) noexcept {
  return {};
}

inline double f$KphpDiv$$getEndTimestamp([[maybe_unused]] const class_instance<C$KphpDiv>& v$this) noexcept {
  return {};
}
