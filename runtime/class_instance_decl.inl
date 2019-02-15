#pragma once

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
  T *o;

  void warn_on_access_null() const;

public:

  inline class_instance();
  inline class_instance(const class_instance<T> &other);
  inline class_instance(class_instance<T> &&other) noexcept;
  inline class_instance(bool value);
  inline class_instance &operator=(const class_instance<T> &other);
  inline class_instance &operator=(class_instance<T> &&other) noexcept;
  inline class_instance &operator=(bool value);
  inline class_instance clone() const;
  inline ~class_instance();

  inline void alloc();
  inline void destroy();
  inline int get_reference_counter() const;

  inline T *operator->() __attribute__ ((always_inline));
  inline T *operator->() const __attribute__ ((always_inline));

  inline bool is_null() const;
  inline string to_string() const;
  inline const char *get_class() const;

  template<class T1>
  friend inline bool eq2(const class_instance<T1> &lhs, const class_instance<T1> &rhs);
  template<class T1>
  friend inline bool equals(const class_instance<T1> &lhs, const class_instance<T1> &rhs);

};
