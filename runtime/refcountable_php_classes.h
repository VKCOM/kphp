#pragma once
#include <cstdint>

#include "runtime/allocator.h"

class abstract_refcountable_php_interface : public ManagedThroughDlAllocator {
public:
  virtual ~abstract_refcountable_php_interface() noexcept = default;
  virtual void add_ref() noexcept = 0;
  virtual void release() noexcept = 0;
  virtual uint32_t get_refcnt() noexcept = 0;
  virtual void set_refcnt(uint32_t new_refcnt) noexcept = 0;
};

template<class Base>
class refcountable_polymorphic_php_classes : public Base {
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

private:
  uint32_t refcnt{0};
};

template<class Derived>
class refcountable_php_classes  : public ManagedThroughDlAllocator {
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

private:
  uint32_t refcnt{0};
};

class refcountable_empty_php_classes {
public:
  static void add_ref() noexcept {}
  static void release() noexcept {}
};
