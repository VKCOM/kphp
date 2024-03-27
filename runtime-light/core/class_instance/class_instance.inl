#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<class T>
class_instance<T> &class_instance<T>::operator=(const Optional<bool> &null) noexcept {
  php_assert(null.value_state() == OptionalState::null_value);
  o.reset();
  return *this;
}

template<class T>
class_instance<T> class_instance<T>::clone_impl(std::true_type /*is empty*/) const {
  return class_instance<T>{}.empty_alloc();
}

template<class T>
class_instance<T> class_instance<T>::clone_impl(std::false_type /*is empty*/) const {
  class_instance<T> res;
  if (o) {
    res.alloc(*o);
    res.o->set_refcnt(1);
  }
  return res;
}

template<class T>
class_instance<T> class_instance<T>::clone() const {
  return clone_impl(std::is_empty<T>{});
}

template<class T>
template<class... Args>
class_instance<T> class_instance<T>::alloc(Args &&... args) {
  static_assert(!std::is_empty<T>{}, "class T may not be empty");
  php_assert(!o);
  new (&o) vk::intrusive_ptr<T>(new T{std::forward<Args>(args)...});
  return *this;
}

template<class T>
inline class_instance<T> class_instance<T>::empty_alloc() {
  static_assert(std::is_empty<T>{}, "class T must be empty");
  new (&o) vk::intrusive_ptr<T>(reinterpret_cast<T*>(dl::allocate(1)));
  return *this;
}

template<class T>
T *class_instance<T>::operator->() {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return get();
};

template<class T>
T *class_instance<T>::operator->() const {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return get();
};

template<class T>
T *class_instance<T>::get() const {
  static_assert(!std::is_empty<T>{}, "class T may not be empty");
  return o.get();
}

template<class T>
void class_instance<T>::warn_on_access_null() const {
  php_warning("Trying to access property of null object");
  const_cast<class_instance<T> *>(this)->alloc();
}

template<class T>
void class_instance<T>::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
  php_assert(o->get_refcnt() == 1);
  o->set_refcnt(static_cast<uint32_t>(static_cast<int32_t>(ref_cnt_value)));
}

template<class T>
bool class_instance<T>::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
  return static_cast<int32_t>(o->get_refcnt()) == ref_cnt_value;
}

template<class T>
void class_instance<T>::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  if (o) {
    php_assert(static_cast<int32_t>(o->get_refcnt()) == expected_ref_cnt);
    o->set_refcnt(1);
    destroy();
  }
}
