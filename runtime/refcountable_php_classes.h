// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cstdint>

#include "common/php-functions.h"

#include "runtime/allocator.h"

class abstract_refcountable_php_interface : public ManagedThroughDlAllocator {
public:
  abstract_refcountable_php_interface() __attribute__((always_inline)) = default;
  virtual ~abstract_refcountable_php_interface() noexcept __attribute__((always_inline)) = default;
  virtual void add_ref() noexcept = 0;
  virtual void release() noexcept = 0;
  virtual uint32_t get_refcnt() noexcept = 0;
  virtual void set_refcnt(uint32_t new_refcnt) noexcept = 0;

  virtual void *get_instance_data_raw_ptr() noexcept = 0;
};

template<class... Bases>
class refcountable_polymorphic_php_classes : public Bases... {
public:
  void add_ref() noexcept final {
    if (refcnt < ExtraRefCnt::for_global_const) {
      ++refcnt;
    }
  }

  uint32_t get_refcnt() noexcept final {
    return refcnt;
  }

  void release() noexcept final __attribute__((always_inline)) {
    if (refcnt < ExtraRefCnt::for_global_const) {
      --refcnt;
    }
    if (refcnt == 0) {
      delete this;
    }
  }

  void set_refcnt(uint32_t new_refcnt) noexcept final {
    refcnt = new_refcnt;
  }

  void *get_instance_data_raw_ptr() noexcept final {
    return this;
  }

private:
  uint32_t refcnt{0};
};

template<class... Interfaces>
class refcountable_polymorphic_php_classes_virt : public virtual abstract_refcountable_php_interface, public Interfaces... {
public:
  refcountable_polymorphic_php_classes_virt() __attribute__((always_inline)) = default;
};

template<>
class refcountable_polymorphic_php_classes_virt<> : public virtual abstract_refcountable_php_interface {
public:
  refcountable_polymorphic_php_classes_virt() __attribute__((always_inline)) = default;

  void add_ref() noexcept final {
    if (refcnt < ExtraRefCnt::for_global_const) {
      ++refcnt;
    }
  }

  uint32_t get_refcnt() noexcept final {
    return refcnt;
  }

  void release() noexcept final __attribute__((always_inline)) {
    if (refcnt < ExtraRefCnt::for_global_const) {
      --refcnt;
    }
    if (refcnt == 0) {
      delete this;
    }
  }

  void set_refcnt(uint32_t new_refcnt) noexcept final {
    refcnt = new_refcnt;
  }

  void *get_instance_data_raw_ptr() noexcept final {
    return this;
  }

private:
  uint32_t refcnt{0};
};

template<class Derived>
class refcountable_php_classes : public ManagedThroughDlAllocator {
public:
  void add_ref() noexcept {
    if (refcnt < ExtraRefCnt::for_global_const) {
      ++refcnt;
    }
  }

  uint32_t get_refcnt() noexcept {
    return refcnt;
  }

  void release() noexcept __attribute__((always_inline)) {
    if (refcnt < ExtraRefCnt::for_global_const) {
      --refcnt;
    }

    if (refcnt == 0) {
      static_assert(!std::is_polymorphic<Derived>{}, "Derived may not be polymorphic");
      /**
       * because of inheritance from ManagedThroughDlAllocator, which override operators new/delete
       * we should have vptr for passing proper sizeof of Derived class, but we don't want to increase size of every class
       * therefore we use static_cast here
       */
      delete static_cast<Derived *>(this);
    }
  }

  void set_refcnt(uint32_t new_refcnt) noexcept {
    refcnt = new_refcnt;
  }

  void *get_instance_data_raw_ptr() noexcept {
    return this;
  }

private:
  uint32_t refcnt{0};
};

class refcountable_empty_php_classes {
public:
  static void add_ref() noexcept {}
  static void release() noexcept {}
};
