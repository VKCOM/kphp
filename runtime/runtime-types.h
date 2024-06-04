#pragma once

#include "runtime-core/runtime-core.h"

#include "runtime/allocator.h"

class ScriptAllocator {
public:
  static void * allocate(int size) {return dl::allocate(size);}
  static void * allocate0(int size) {return dl::allocate0(size);}
  static void deallocate(void * p, int size) {dl::deallocate(p, size);}
  static void * reallocate(void * p, int new_size, int old_size){return dl::reallocate(p, new_size, old_size);}
};

class HeapAllocator {
public:
  static void * allocate(int size) {return dl::heap_allocate(size);}
  static void * allocate0(int size) {
    void * ptr = dl::heap_allocate(size);
    memset(ptr, 0, size);
    return ptr;}
  static void deallocate(void * p, int size) {dl::heap_deallocate(p, size);}
  static void * reallocate(void * p, int new_size, int old_size){return dl::heap_reallocate(p, new_size, old_size);}
};

using string = __string<ScriptAllocator>;
using tmp_string = __runtime_core::tmp_string<ScriptAllocator>;
using mixed = __mixed<ScriptAllocator>;
using string_buffer = __string_buffer<HeapAllocator>;
template<typename T>
using array = __array<T, ScriptAllocator>;
template<typename T>
using class_instance = __class_instance<T, ScriptAllocator>;
using abstract_refcountable_php_interface = __runtime_core::abstract_refcountable_php_interface<ScriptAllocator>;
template<typename ...Bases>
using refcountable_polymorphic_php_classes = __runtime_core::refcountable_polymorphic_php_classes<Bases...>;
template<typename ...Interfaces>
using refcountable_polymorphic_php_classes_virt = __runtime_core::refcountable_polymorphic_php_classes_virt<ScriptAllocator, Interfaces...>;
template<class Derived>
using refcountable_php_classes = __runtime_core::refcountable_php_classes<Derived, ScriptAllocator>;
using refcountable_empty_php_classes = __runtime_core::refcountable_empty_php_classes;

  template<>
template<class T1, class, class>
string convert_to<string>::convert(T1 &&val) {
  return f$strval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
array<mixed> convert_to<array<mixed>>::convert(T1 &&val) {
  return f$arrayval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
mixed convert_to<mixed>::convert(T1 &&val) {
  return mixed{std::forward<T1>(val)};
}

inline string f$strval(bool val) {
  return (val ? string("1", 1) : string());
}

inline string f$strval(int64_t val) {
  return string(val);
}

inline string f$strval(double val) {
  return string(val);
}

template<class T>
inline string f$strval(const Optional<T> &val) {
  return val.has_value() ? f$strval(val.val()) : f$strval(false);
}

template<class T>
inline string f$strval(Optional<T> &&val) {
  return val.has_value() ? f$strval(std::move(val.val())) : f$strval(false);
}

template<class T>
inline array<T> f$arrayval(const T &val) {
  array<T> res(array_size(1, true));
  res.push_back(val);
  return res;
}

template<class T>
inline array<T> f$arrayval(const Optional<T> &val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(val.val());
    case OptionalState::false_value:
      return impl_::false_cast_to_array<T, ScriptAllocator>();
    case OptionalState::null_value:
      return array<T>{};
    default:
      __builtin_unreachable();
  }
}

template<class T>
inline array<T> f$arrayval(Optional<T> &&val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(std::move(val.val()));
    case OptionalState::false_value:
      return impl_::false_cast_to_array<T, ScriptAllocator>();
    case OptionalState::null_value:
      return array<T>{};
    default:
      __builtin_unreachable();
  }
}

template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, const Optional<T> &opt, Args &&...args) {
  return __runtime_core::call_fun_on_optional_value<ScriptAllocator>(std::forward<FunT>(fun), opt, std::forward<Args>(args)...);
}
template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, Optional<T> &&opt, Args &&...args) {
  return __runtime_core::call_fun_on_optional_value<ScriptAllocator>(std::forward<FunT>(fun), opt, std::forward<Args>(args)...);
}
