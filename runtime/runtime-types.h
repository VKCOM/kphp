#pragma once

#include "runtime-core/runtime-core.h"

#include "runtime/allocator.h"

struct DefaultAllocator {
public:
  static void * allocate(int size) {return dl::allocate(size);}
  static void * allocate0(int size) {return dl::allocate0(size);}
  static void deallocate(void * p, int size) {dl::deallocate(p, size);}
  static void * reallocate(void * p, int new_size, int old_size){return dl::reallocate(p, new_size, old_size);}
};

using ScriptAllocator = DefaultAllocator;

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
using array_iterator = __runtime_core::array_iterator<T, ScriptAllocator>;
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


template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, const Optional<T> &opt, Args &&...args) {
  return __runtime_core::call_fun_on_optional_value<ScriptAllocator>(std::forward<FunT>(fun), opt, std::forward<Args>(args)...);
}
template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, Optional<T> &&opt, Args &&...args) {
  return __runtime_core::call_fun_on_optional_value<ScriptAllocator>(std::forward<FunT>(fun), opt, std::forward<Args>(args)...);
}

inline mixed operator+(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator+(lhs, rhs);
}

inline mixed operator-(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator-(lhs, rhs);
}

inline mixed operator*(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator*(lhs, rhs);
}

inline mixed operator-(const string &lhs) {
  return __runtime_core::operator-(lhs);
}

inline mixed operator+(const string &lhs) {
  return __runtime_core::operator+(lhs);
}

inline int64_t operator&(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator&(lhs, rhs);
}

inline int64_t operator|(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator|(lhs, rhs);
}

inline int64_t operator^(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator^(lhs, rhs);
}

inline int64_t operator<<(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator<<(lhs, rhs);
}

inline int64_t operator>>(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator>>(lhs, rhs);
}

inline bool operator<(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator<(lhs, rhs);
}

inline bool operator<=(const mixed &lhs, const mixed &rhs) {
  return __runtime_core::operator<=(lhs, rhs);
}

template<class T>
inline string f$get_type(const T &v) {
  return __runtime_core::get_type<ScriptAllocator>(v);
}

inline string f$get_class(bool v) {
  return __runtime_core::get_class<ScriptAllocator>(v);
}

inline string f$get_class(int64_t v) {
  return __runtime_core::get_class<ScriptAllocator>(v);
}
template<typename Allocator>
inline string f$get_class(double v) {
  return __runtime_core::get_class<ScriptAllocator>(v);
}
