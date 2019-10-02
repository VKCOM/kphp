#pragma once

#include "common/smart_ptrs/intrusive_ptr.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

// из php-шных классов генерируются С++ структуры такого вида
// а для их инстанцирования — class_instance<T>

//struct C$Classes$A {
//  int ref_cnt;
//  int $a;
//  string $str;
//  array <int> $intArr;
//
//  inline const char *get_class() const { return "Classes\\A"; }
//};

template<class T>
class class_instance {
  vk::intrusive_ptr<T> o;

  void warn_on_access_null() const;

public:
  using ClassType = T;

  class_instance() = default;
  class_instance(const class_instance &) = default;
  class_instance(class_instance &&) noexcept = default;

  class_instance(bool) {}

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  class_instance(const class_instance<Derived> &d)
    : o(d.o) {
  }

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  class_instance(class_instance<Derived> &&d) noexcept
    : o(std::move(d.o)) {
  }

  class_instance& operator=(const class_instance &) = default;
  class_instance& operator=(class_instance &&) noexcept = default;

  // prohibits creating a class_instance from int/char*/etc by implicit casting them to bool
  template<class T2>
  class_instance(T2) = delete;

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  class_instance& operator=(const class_instance<Derived> &d) {
    o = d.o;
    return *this;
  }

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  class_instance& operator=(class_instance<Derived> &&d) noexcept {
    o = std::move(d.o);
    return *this;
  }

  // prohibits assignment int/char*/etc to class_instance by implicit casting them to bool
  template<class T2>
  class_instance &operator=(T2) = delete;

  inline class_instance &operator=(bool);
  inline class_instance clone() const;
  template<class... Args>
  inline class_instance<T> alloc(Args &&... args) __attribute__((always_inline));
  inline class_instance<T> empty_alloc() __attribute__((always_inline));
  inline void destroy() { o.reset(); }
  int get_reference_counter() const { return o->get_refcnt(); }

  void set_reference_counter_to_cache();
  void destroy_cached();

  constexpr static dl::size_type estimate_memory_usage() {
    static_assert(!std::is_abstract<T>::value, "forbidden for interfaces");
    static_assert(!std::is_empty<T>{}, "class T may not be empty");
    return sizeof(T);
  }

  inline T *operator->() __attribute__ ((always_inline));
  inline T *operator->() const __attribute__ ((always_inline));

  inline T *get() const __attribute__ ((always_inline));

  bool is_null() const { return !static_cast<bool>(o); }
  const char *get_class() const { return o ? o->get_class() : "null"; }

  template<class D>
  bool is_a() const {
    return is_a_helper<D, T>();
  }

  template<class D, class CurType, class Derived = std::enable_if_t<std::is_polymorphic<CurType>{}, D>, class dummy = void>
  bool is_a_helper() const {
    return dynamic_cast<Derived *>(o.get());
  }

  template<class D, class CurType, class Derived = std::enable_if_t<!std::is_polymorphic<CurType>{}, D>>
  bool is_a_helper() const {
    return o && std::is_same<T, Derived>{};
  }

  template<class Derived>
  class_instance<Derived> cast_to() const {
    class_instance<Derived> res;
    res.o = vk::dynamic_pointer_cast<Derived>(o);
    return res;
  }

  template<class T1>
  friend inline bool eq2(const class_instance<T1> &lhs, const class_instance<T1> &rhs);

  template<class T1>
  friend inline bool equals(const class_instance<T1> &lhs, const class_instance<T1> &rhs);

  template<class Derived>
  friend class class_instance;

private:
  class_instance<T> clone_impl(std::true_type /*is empty*/) const;
  class_instance<T> clone_impl(std::false_type /*is empty*/) const;
};

template<typename>
struct is_class_instance : std::false_type {
};

template<typename T>
struct is_class_instance<class_instance<T>> : std::true_type {
};
