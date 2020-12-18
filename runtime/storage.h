// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

extern const char *last_wait_error;

struct thrown_exception {
  Throwable exception;
  thrown_exception() = default;
  explicit thrown_exception(Throwable exception) noexcept : exception(std::move(exception)) {}
};

template<size_t limit>
union small_obect_ptr {
  char storage_[limit];
  void *storage_ptr;

  template <typename T, typename ...Args>
  std::enable_if_t<sizeof(T) <= limit, T*> emplace(Args&& ...args) noexcept {
    return new (storage_) T(std::forward<Args>(args)...);
  }
  template <typename T>
  std::enable_if_t<sizeof(T) <= limit, T*> get() noexcept {
    return reinterpret_cast<T*>(storage_);
  }
  template <typename T>
  std::enable_if_t<sizeof(T) <= limit> destroy() noexcept {
    get<T>()->~T();
  }

  template <typename T, typename ...Args>
  std::enable_if_t<limit < sizeof(T), T*> emplace(Args&& ...args) noexcept {
    storage_ptr = dl::allocate(sizeof(T));
    return new (storage_ptr) T(std::forward<Args>(args)...);
  }
  template <typename T>
  std::enable_if_t<limit < sizeof(T), T*> get() noexcept {
    return static_cast<T*>(storage_ptr);
  }
  template <typename T>
  std::enable_if_t<limit < sizeof(T)> destroy() noexcept {
    T *mem = get<T>();
    mem->~T();
    dl::deallocate(mem, sizeof(T));
  }
};

class Storage {
private:
  using storage_ptr = small_obect_ptr<sizeof(mixed)>;
  storage_ptr storage_;

  template<class X, class Y, class Tag = typename std::is_convertible<X, Y>::type>
  struct load_implementation_helper;

  void save_exception() noexcept;

public:

  // this class specializations are generated by kphp compiler

  template<typename T>
  struct tagger {
    static_assert(!std::is_same<T, int>{}, "int is forbidden");
    static int get_tag() noexcept;
  };

  template<typename T>
  struct loader {
    static_assert(!std::is_same<T, int>{}, "int is forbidden");
    using loader_fun = T(*)(storage_ptr &);
    static loader_fun get_function(int tag) noexcept;
  };

  int tag;

  Storage() noexcept;

  /**
   * enabled_if is used to disable type deduction for save function
   * It should be called with exacty same type as load,
   * and it would be too easy to make a bug, if it's deduced automatically
   */
  template<class T1>
  void save(std::enable_if_t<true, T1> x) noexcept;

  void save_void() noexcept;

  template<class X>
  X load() noexcept;

  template<class X>
  X load_as() noexcept;
};


template<class X, class Y>
struct Storage::load_implementation_helper<X, Y, std::false_type> {
  static Y load(storage_ptr &) noexcept {
    php_assert(0);      // should be never called in runtime, used just to prevent compilation errors
    return Y();
  }
};

template<class X, class Y>
struct Storage::load_implementation_helper<X, Y, std::true_type> {
  static_assert(!std::is_same<X, int>{}, "int is forbidden");
  static_assert(!std::is_same<Y, int>{}, "int is forbidden");

  static Y load(storage_ptr &storage) noexcept {
    X *data = storage.get<X>();
    Y result = std::move(*data);
    storage.destroy<X>();
    return result;
  }
};

template<>
struct Storage::load_implementation_helper<void, void, std::true_type> {
  static void load(storage_ptr &) noexcept {}
};

template<typename T>
struct Storage::load_implementation_helper<T, void, std::false_type> {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  static void load(storage_ptr &storage) noexcept {
    Storage::load_implementation_helper<T, T>::load(storage);
  }
};

template<class Y>
struct Storage::load_implementation_helper<thrown_exception, Y, std::false_type> {
  static_assert(!std::is_same<Y, int>{}, "int is forbidden");

  static Y load(storage_ptr &storage) noexcept {
    php_assert (CurException.is_null());
    CurException = load_implementation_helper<thrown_exception, thrown_exception>::load(storage).exception;
    return Y();
  }
};

template<>
struct Storage::load_implementation_helper<thrown_exception, void, std::false_type> {
  static void load(storage_ptr &storage) noexcept {
    php_assert (CurException.is_null());
    CurException = load_implementation_helper<thrown_exception, thrown_exception>::load(storage).exception;
  }
};



template<class T1>
void Storage::save(std::enable_if_t<true, T1> x) noexcept {
  static_assert(!std::is_same<T1, int>{}, "int is forbidden");

  if (!CurException.is_null()) {
    save_exception();
  } else {
    storage_.emplace<T1>(std::move(x));
    tag = tagger<T1>::get_tag();
  }
}

template<class X>
X Storage::load() noexcept {
  static_assert(!std::is_same<X, int>{}, "int is forbidden");

  php_assert (tag != 0);
  if (tag == tagger<thrown_exception>::get_tag()) {
    tag = 0;
    return load_implementation_helper<thrown_exception, X>::load(storage_);
  }

  php_assert(tag == tagger<X>::get_tag());
  tag = 0;
  return load_implementation_helper<X, X>::load(storage_);
}


template<class X>
X Storage::load_as() noexcept {
  static_assert(!std::is_same<X, int>{}, "int is forbidden");

  php_assert (tag != 0);

  int tag_save = tag;
  tag = 0;
  return (loader<X>::get_function(tag_save))(storage_);
}
