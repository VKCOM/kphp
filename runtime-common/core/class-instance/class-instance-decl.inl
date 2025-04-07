#pragma once

#include "common/smart_ptrs/intrusive_ptr.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

// PHP classes produce the C++ structures of the form:
//
//struct C$Classes$A {
//  int ref_cnt;
//  int $a;
//  string $str;
//  array <int> $intArr;
//
//  inline const char *get_class() const { return "Classes\\A"; }
//};
//
// Their instances are wrapped into the class_instance<T>.

class abstract_refcountable_php_interface;

template<class T>
class class_instance {


  void warn_on_access_null() const;

public:
  vk::intrusive_ptr<T> o;
  using ClassType = T;

  class_instance() = default;
  class_instance(const class_instance &) = default;
  class_instance(class_instance &&) noexcept = default;

  class_instance(const Optional<bool> &null) noexcept {
    php_assert(null.value_state() == OptionalState::null_value);
  }

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

  inline class_instance &operator=(const Optional<bool> &null) noexcept;
  inline class_instance clone() const;
  template<class... Args>
  inline class_instance<T> alloc(Args &&... args) __attribute__((always_inline));
  inline class_instance<T> empty_alloc() __attribute__((always_inline));
  inline void destroy() { o.reset(); }
  int64_t get_reference_counter() const { return o ? o->get_refcnt() : 0; }

  void set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept;
  bool is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept;
  void force_destroy(ExtraRefCnt expected_ref_cnt) noexcept;

  template<class S = T>
  std::enable_if_t<!std::is_polymorphic<S>{}, size_t> estimate_memory_usage() const noexcept {
    static_assert(!std::is_empty<T>{}, "class T may not be empty");
    return sizeof(T);
  }

  template<class S = T>
  std::enable_if_t<std::is_polymorphic<S>{}, size_t> estimate_memory_usage() const noexcept {
    // TODO this is used only for job workers. Should we use this logic for other?
    return o->virtual_builtin_sizeof();
  }

  template<class S = T>
  std::enable_if_t<!std::is_polymorphic<S>{}, class_instance> virtual_builtin_clone() const noexcept {
    return clone();
  }

  template<class S = T>
  std::enable_if_t<std::is_polymorphic<S>{}, class_instance> virtual_builtin_clone() const noexcept {
    // TODO this is used only for job workers. Should we use this logic for other?
    class_instance res;
    if (o) {
      res.o = vk::intrusive_ptr<T>{o->virtual_builtin_clone()};
      res.o->set_refcnt(1);
    }
    return res;
  }

  template<class S = T>
  std::enable_if_t<!std::is_polymorphic<S>{}, void *> get_base_raw_ptr() const noexcept {
    return get();
  }

  template<class S = T>
  std::enable_if_t<std::is_polymorphic<S>{}, void *> get_base_raw_ptr() const noexcept {
    // all polymorphic instances inherit abstract_refcountable_php_interface
    // don't inline, we need an explicit conversion to abstract_refcountable_php_interface
    abstract_refcountable_php_interface *interface = get();
    return interface;
  }

  template<class S = T>
  static std::enable_if_t<!std::is_polymorphic<S>{}, class_instance> create_from_base_raw_ptr(void *raw_ptr) noexcept {
    class_instance res;
    res.o = vk::intrusive_ptr<T>{static_cast<T *>(raw_ptr)};
    return res;
  }

  template<class S = T>
  static std::enable_if_t<std::is_polymorphic<S>{}, class_instance> create_from_base_raw_ptr(void *raw_ptr) noexcept {
    class_instance res;
    auto *interface = static_cast<abstract_refcountable_php_interface *>(raw_ptr);
    auto *object = dynamic_cast<T *>(interface);
    php_assert(object);
    res.o = vk::intrusive_ptr<T>{object};
    return res;
  }

  template<class S = T>
  static std::enable_if_t<std::is_polymorphic<S>{}, class_instance> create_from_polymorphic(T *object) noexcept {
    class_instance res;
    php_assert(object);
    res.o = vk::intrusive_ptr<T>{object};
    return res;
  }

  inline T *operator->() __attribute__ ((always_inline));
  inline T *operator->() const __attribute__ ((always_inline));

  inline T *get() const __attribute__ ((always_inline));

  bool is_null() const { return !static_cast<bool>(o); }
  const char *get_class() const { return o ? o->get_class() : "null"; }
  int64_t get_hash() const { return o ? o->get_hash() : 0; }

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

  inline bool operator==(const class_instance<T> &rhs) const {
    return o == rhs.o;
  }

  template<class Derived>
  friend class class_instance;

private:
  class_instance<T> clone_impl(std::true_type /*is empty*/) const;
  class_instance<T> clone_impl(std::false_type /*is empty*/) const;
};

template<class T, class ...Args>
class_instance<T> make_instance(Args &&...args) noexcept {
  class_instance<T> instance;
  instance.alloc(std::forward<Args>(args)...);
  return instance;
}
