// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once


#include "runtime/memory_usage.h"
#include "runtime/refcountable_php_classes.h"
#include "runtime/to-array-processor.h"
#include "runtime/instance-copy-processor.h"

struct C$UnitEnum : public abstract_refcountable_php_interface {
    virtual const char *get_class() const  noexcept  = 0;
    virtual int get_hash() const  noexcept  = 0;

    virtual void accept(ToArrayVisitor &) noexcept {}
    virtual void accept(InstanceMemoryEstimateVisitor &) noexcept {}
    virtual void accept(InstanceReferencesCountingVisitor &) noexcept {}
    virtual void accept(InstanceDeepCopyVisitor &) noexcept {}
    virtual void accept(InstanceDeepDestroyVisitor &) noexcept {}

    C$UnitEnum() __attribute__((always_inline)) = default;
    ~C$UnitEnum() __attribute__((always_inline)) = default;
};

struct C$BackedEnum : public C$UnitEnum {
  virtual const char *get_class() const  noexcept  override  = 0;
  virtual int get_hash() const  noexcept  override  = 0;

  C$BackedEnum() __attribute__((always_inline)) = default;
  ~C$BackedEnum() __attribute__((always_inline)) = default;
};
