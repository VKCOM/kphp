#pragma once

/*
 *
 *   Do not include with file directly
 *   Include kphp_core.h instead
 *
 */

class stdClass {
  typedef array <var, var>::array_inner array_inner;
  typedef array <var, var>::string_hash_entry string_hash_entry;

  array_inner *data;

  inline void mutate_if_needed (void);

  inline static array_inner *create (int new_size);

public:
  inline stdClass (void);

  
  inline virtual const char* get_class (void) const;

  inline virtual array <string> sleep (void);

  inline virtual void wakeup (void);

  inline virtual void set (const string &key, const var &value);

  inline virtual var get (const string &key);

  inline virtual bool isset (const string &key);

  inline virtual void unset (const string &key);

  inline virtual string to_string (void);

  inline virtual array <var> to_array (void);

  inline virtual var call (const string &name, const array <var> &params);


  inline stdClass (const stdClass &other);

  inline stdClass& operator = (const stdClass &other);

  inline virtual ~stdClass (void);


  template <class T>
  friend class object_ptr;

  template <class T, class TT>
  friend class array;
};

template <class T>
class object_ptr {
  T *o;

public:
  inline object_ptr (void);
  
  inline object_ptr (const object_ptr &other);

  template <class T1>
  inline object_ptr (const object_ptr <T1> &other);

  inline object_ptr& operator = (const object_ptr &other);

  template <class T1>
  inline object_ptr& operator = (const object_ptr <T1> &other);

  inline void destroy (void);
  inline ~object_ptr (void);


  T& operator *();
  const T& operator *() const;

  T* operator ->();
  const T* operator ->() const;


  inline array <string> sleep (void);

  inline void wakeup (void);

  inline void set (const string &key, const var &value);

  inline var get (const string &key);

  inline bool isset (const string &key);

  inline void unset (const string &key);

  
  inline string to_string (void);

  inline array <var> to_array (void);

  inline void swap (object_ptr &other);


  int get_reference_counter (void) const;

  
  template <class T1>
  friend inline bool eq2 (const object_ptr <T1> &lhs, const object_ptr <T1> &rhs);

  template <class T1, class T2>
  friend inline bool eq2 (const object_ptr <T1> &lhs, const object_ptr <T2> &rhs);

  template <class T1>
  friend inline bool equals (const object_ptr <T1> &lhs, const object_ptr <T1> &rhs);

  template <class T1, class T2>
  friend inline bool equals (const object_ptr <T1> &lhs, const object_ptr <T2> &rhs);
  
  
  template <class T1>
  friend class object_ptr;

  template <class T1, class TT>
  friend class array;
};


template <class T>
inline void swap (object_ptr <T> &lhs, object_ptr <T> &rhs);

template <class T>
inline bool eq2 (const object_ptr <T> &lhs, const object_ptr <T> &rhs);

template <class T1, class T2>
inline bool eq2 (const object_ptr <T1> &lhs, const object_ptr <T2> &rhs);

template <class T>
inline bool equals (const object_ptr <T> &lhs, const object_ptr <T> &rhs);

template <class T1, class T2>
inline bool equals (const object_ptr <T1> &lhs, const object_ptr <T2> &rhs);

