#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<class T>
class_instance<T>::class_instance() :
  o(NULL) {
}

template<class T>
class_instance<T>::class_instance(const class_instance<T> &other) :
  o(other.o) {
  if (o) {
    o->ref_cnt++;
  }
}

template<class T>
class_instance<T>::class_instance(bool value __attribute__((unused))) :
  o(NULL) {
}

template<class T>
class_instance<T> &class_instance<T>::operator=(const class_instance<T> &other) {
  if (other.o) {
    other.o->ref_cnt++;
  }
  destroy();
  o = other.o;
  return *this;
}

template<class T>
class_instance<T> &class_instance<T>::operator=(bool value __attribute__((unused))) {
  destroy();
  return *this;
}

template<class T>
void class_instance<T>::alloc() {
  php_assert(o == NULL);
  o = static_cast <T *> (dl::allocate(sizeof(T)));
  new(o) T();
}

template<class T>
void class_instance<T>::destroy() {
  if (o != NULL && o->ref_cnt-- <= 0) {
    o->~T();
    dl::deallocate(o, sizeof(T));
  }
  o = NULL;
}

template<class T>
class_instance<T>::~class_instance() {
  destroy();
}

template<class T>
int class_instance<T>::get_reference_counter() const {
  return o->ref_cnt + 1;
}


template<class T>
bool class_instance<T>::is_null() const {
  return o == NULL;
}

template<class T>
string class_instance<T>::to_string() const {
  const char *classname = get_class();
  return string(classname, (int)strlen(classname));
}

template<class T>
const char *class_instance<T>::get_class() const {
  return o != NULL ? o->get_class() : "null";
}


template<class T>
T *class_instance<T>::operator->() {
  if (unlikely(o == NULL)) {
    warn_on_access_null();
  }

  return o;
};

template<class T>
T *class_instance<T>::operator->() const {
  if (unlikely(o == NULL)) {
    warn_on_access_null();
  }

  return o;
};

template<class T>
void class_instance<T>::warn_on_access_null() const {
  php_warning("Trying to access property of null object");
  const_cast<class_instance<T> *>(this)->alloc();
}


