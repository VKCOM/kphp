#pragma once

#include <memory>

#include "runtime/allocator.h"
#include "runtime/php_assert.h"

template<typename T>
class unique_object {
public:
  unique_object() = default;

  template<typename S>
  unique_object(unique_object<S> &&other) :
    unique_object(other.ptr_.release()) {
    static_assert(std::is_base_of<T, S>::value, "cast to base allowed");
  }

  void reset() { ptr_.reset(); }
  T *operator->() { return ptr_.operator->(); }
  explicit operator bool() const { return !!ptr_; }

private:
  template<typename S>
  friend class unique_object;

  template<class S, class... Args>
  friend unique_object<S> make_unique_object(Args &&... args);

  static constexpr size_t MAX_MEMORY = 1024u;

  struct UniqueObjectDeleter {
    void operator()(T *obj) const {
      // dynamic_cast<void*> нужен, чтобы получить реальный адрес выделенного объекта
      // (в случае множественного наследования, после move в одну из баз T, они могут не совпадать)
      void *mem = reinterpret_cast<uint8_t *>(dynamic_cast<void*>(obj)) - sizeof(size_t);
      obj->~T();
      const size_t allocated_size = *static_cast<size_t *>(mem);
      php_assert(allocated_size >= sizeof(T));
      php_assert(allocated_size <= MAX_MEMORY);
      dl::deallocate(mem, allocated_size);
    }
  };

  explicit unique_object(T *ptr) :
    ptr_(ptr, UniqueObjectDeleter{}) {}

  std::unique_ptr<T, UniqueObjectDeleter> ptr_;
};

template<typename T, class... Args>
unique_object<T> make_unique_object(Args &&... args) {
  constexpr size_t allocated_size = sizeof(size_t) + sizeof(T);
  static_assert(allocated_size <= unique_object<T>::MAX_MEMORY, "Too big size to be allocated");

  void *mem = dl::allocate(allocated_size);
  *static_cast<size_t *>(mem) = allocated_size;
  T *new_t = reinterpret_cast<T *>(static_cast<uint8_t *>(mem) + sizeof(size_t));

  new(new_t) T(std::forward<Args>(args)...);
  return unique_object<T>{new_t};
}
