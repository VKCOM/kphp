// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime-common/core/class-instance/refcountable-php-classes.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

struct C$ArrayAccess : public may_be_mixed_base {
  virtual int get_hash() const noexcept = 0;

  C$ArrayAccess() __attribute__((always_inline)) = default;
  ~C$ArrayAccess() override __attribute__((always_inline)) = default;
};


bool f$ArrayAccess$$offsetExists(class_instance<C$ArrayAccess> const & /*v$this*/, mixed const & /*v$offset*/) noexcept;
mixed f$ArrayAccess$$offsetGet(class_instance<C$ArrayAccess> const & /*v$this*/, mixed const & /*v$offset*/) noexcept;
void f$ArrayAccess$$offsetSet(class_instance<C$ArrayAccess> const & /*v$this*/, mixed const & /*v$offset*/, mixed const & /*v$value*/) noexcept;
void f$ArrayAccess$$offsetUnset(class_instance<C$ArrayAccess> const & /*v$this*/, mixed const & /*v$offset*/) noexcept;
