// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

struct C$KphpSpanEvent : public refcountable_php_classes<C$KphpSpanEvent> {
  int span_id{0};

  C$KphpSpanEvent() = default;
  explicit C$KphpSpanEvent(int span_id)
    : span_id(span_id) {}
};
