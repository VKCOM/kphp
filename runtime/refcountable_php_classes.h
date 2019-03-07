#pragma once
#include <cstdint>

#include "runtime/allocator.h"

template<class Derived, class Base = void>
class refcountable_php_classes : public Base {
public:
  void add_ref() final {
    ++refcnt;
  }

  uint32_t get_refcnt() final {
    return refcnt;
  }

  void release() final __attribute__((always_inline)) {
    refcnt--;
    if (refcnt == 0) {
      auto derived = static_cast<Derived *>(this);
      derived->~Derived();
      dl::deallocate(derived, sizeof(Derived));
    }
  }

  void set_refcnt(uint32_t new_refcnt) final {
    refcnt = new_refcnt;
  }

private:
  uint32_t refcnt{0};
};

template<class Derived>
class refcountable_php_classes<Derived, void> {
public:
  void add_ref() {
    ++refcnt;
  }

  uint32_t get_refcnt() {
    return refcnt;
  }

  void release() __attribute__((always_inline)) {
    refcnt--;
    if (refcnt == 0) {
      auto derived = static_cast<Derived *>(this);
      derived->~Derived();
      dl::deallocate(derived, sizeof(Derived));
    }
  }

  void set_refcnt(uint32_t new_refcnt) {
    refcnt = new_refcnt;
  }

private:
  uint32_t refcnt{0};
};
