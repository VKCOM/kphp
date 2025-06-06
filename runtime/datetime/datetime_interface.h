// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/class-instance/refcountable-php-classes.h"

#include "runtime/datetime/timelib_wrapper.h"

struct C$DateTimeInterface : public refcountable_polymorphic_php_classes_virt<> {
  timelib_time* time{nullptr};

  virtual const char* get_class() const noexcept = 0;
  virtual int get_hash() const noexcept = 0;
};
