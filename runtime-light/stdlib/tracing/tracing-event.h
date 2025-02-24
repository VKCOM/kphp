// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/class-instance/refcountable-php-classes.h"

struct C$KphpSpanEvent : public refcountable_php_classes<C$KphpSpanEvent> {
  int32_t span_id{0};

  C$KphpSpanEvent() = default;
  explicit C$KphpSpanEvent(int32_t span_id)
    : span_id(span_id) {}
};
