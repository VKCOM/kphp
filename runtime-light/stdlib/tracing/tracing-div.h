// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/visitors/common-visitors-methods.h"
#include "runtime-light/stdlib/tracing/tracing-event.h"

struct C$KphpDiv : public refcountable_php_classes<C$KphpDiv>, private CommonDefaultVisitorMethods {
  using CommonDefaultVisitorMethods::accept;
};
