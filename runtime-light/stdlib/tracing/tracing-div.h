// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$KphpDiv : public refcountable_php_classes<C$KphpDiv>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;
};
