#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<class T>
class_instance<T> &class_instance<T>::operator=(bool) {
  o.reset();
  return *this;
}

template<class T>
class_instance<T> class_instance<T>::clone() const {
  class_instance<T> res;
  if (o) {
    res.alloc();
    *res.o = *o;
    res.o->set_refcnt(1);
  }

  return res;
}

template<class T>
void class_instance<T>::alloc() {
  php_assert(!o);
  auto new_t = static_cast<T *>(dl::allocate(sizeof(T)));
  new(new_t) T();
  new (&o) vk::intrusive_ptr<T>(new_t);
}

template<class T>
T *class_instance<T>::operator->() {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return o.get();
};

template<class T>
T *class_instance<T>::operator->() const {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return o.get();
};

template<class T>
T *class_instance<T>::get() const {
  return o.get();
}

template<class T>
void class_instance<T>::warn_on_access_null() const {
  php_warning("Trying to access property of null object");
  const_cast<class_instance<T> *>(this)->alloc();
}

template<class T>
void class_instance<T>::set_reference_counter_to_cache() {
  php_assert(o->get_refcnt() == 1);
  o->set_refcnt(REF_CNT_FOR_CACHE);
}

template<class T>
void class_instance<T>::destroy_cached() {
  if (o) {
    php_assert(o->get_refcnt() == REF_CNT_FOR_CACHE);
    o->set_refcnt(1);
    destroy();
  }
}
