#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

class Storage {
private:
  char storage_[sizeof(var)];

  template<class X, class Y, class Tag = typename std::is_convertible<X, Y>::type>
  struct load_implementation_helper;

  static var load_exception(char *storage);

  void save_exception();

public:
  using Getter = var (*)(char *);

  Getter getter_;

  Storage();

  template<class T1, class T2>
  void save(const T2 &x, Getter getter);

  template<class T1, class T2>
  void save(const T2 &x);

  void save_void();

  template<class X>
  X load();

  var load_as_var();
};


template<class X>
struct Storage::load_implementation_helper<X, var, std::false_type> {
  static var load(char *storage __attribute__((unused))) {
    php_assert(0);      // should be never called in runtime, used just to prevent compilation errors
    return var();
  }
};

template<class X, class Y>
struct Storage::load_implementation_helper<X, Y, std::true_type> {
  static Y load(char *storage) {
    if (sizeof(X) > sizeof(storage_)) {
      // какие-нибудь длинные tuple'ы (см. save())
      // тогда в storage лежит указатель на выделенную память
      storage = static_cast<char *>(*reinterpret_cast<void **>(storage));
    }
    X *data = reinterpret_cast <X *> (storage);
    Y result = *data;
    data->~X();
    if (sizeof(X) > sizeof(storage_)) {
      dl::deallocate(storage, sizeof(X));
    }
    return result;
  }
};

template<>
struct Storage::load_implementation_helper<void, void, std::true_type> {
  static void load(char *storage __attribute__((unused))) {}
};

template<>
struct Storage::load_implementation_helper<void, var, std::false_type> {
  static var load(char *storage __attribute__((unused))) { return var(); }
};


template<class T1, class T2>
void Storage::save(const T2 &x, Getter getter) {
  if (!CurException.is_null()) {
    save_exception();
  } else {
    if (sizeof(T1) <= sizeof(storage_)) {
      #pragma GCC diagnostic push
      #if __GNUC__ >= 6
        #pragma GCC diagnostic ignored "-Wplacement-new="
      #endif
      new(storage_) T1(x);
      #pragma GCC diagnostic pop
    } else {
      // какие-нибудь длинные tuple'ы, которые не влазят в var
      // для них выделяем память отдельно, а в storage сохраняем указатель
      void *mem = dl::allocate(sizeof(T1));
      new(mem) T1(x);
      *reinterpret_cast<void **>(storage_) = mem;
    }

    getter_ = getter;
    php_assert (getter_ != nullptr);
  }
}

template<class T1, class T2>
void Storage::save(const T2 &x) {
  save<T1, T2>(x, load_implementation_helper<T1, var>::load);
}

template<class X>
X Storage::load() {
  php_assert (getter_ != nullptr);
  if (getter_ == load_exception) {
    getter_ = nullptr;
    load_exception(storage_);
    return X();
  }

  getter_ = nullptr;
  return load_implementation_helper<X, X>::load(storage_);
}
