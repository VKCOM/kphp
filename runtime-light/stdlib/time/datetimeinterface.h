// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp/timelib/timelib.h"
#include "timelib-functions.h"

#include "runtime-common/core/class-instance/refcountable-php-classes.h"

struct C$DateTimeInterface : public refcountable_polymorphic_php_classes_virt<> {
  kphp::timelib::time_t time{nullptr};

  virtual const char* get_class() const noexcept = 0;
  virtual int32_t get_hash() const noexcept = 0;
};
