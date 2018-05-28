#pragma once

/*
 *
 *   Do not include with file directly
 *   Include kphp_core.h instead
 *
 */


// из php-шных классов генерируются С++ структуры такого вида
// а для их инстанцирования — class_instance<T>

//struct C$Classes$A {
//  int $ref_cnt;
//  int a;
//  string str;
//  array <int> intArr;
//
//  inline const char *get_class() const { return "Classes\\A"; }
//};

template <class T>
class class_instance {
  T *o;

public:

  inline class_instance ();
  inline class_instance (const class_instance <T> &other);
  inline class_instance (bool value);
  inline class_instance &operator= (const class_instance <T> &other);
  inline class_instance &operator= (bool value);
  inline ~class_instance ();

  inline void alloc ();
  inline void destroy ();
  inline int get_reference_counter () const;

  T *operator-> ();
  T *operator-> () const;

  inline bool is_null () const;
  inline string to_string () const;
  inline const char *get_class () const;

  template <class T1>
  friend inline bool eq2 (const class_instance <T1> &lhs, const class_instance <T1> &rhs);
  template <class T1>
  friend inline bool equals (const class_instance <T1> &lhs, const class_instance <T1> &rhs);

};
