// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_INTRUSIVE_PTR_H
#define ENGINE_INTRUSIVE_PTR_H

#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>

namespace vk {

template <class T>
class intrusive_ptr {
public:
  intrusive_ptr() noexcept = default;

  explicit intrusive_ptr(T *ptr, bool add_ref = true) : ptr{ptr} {
    if (add_ref && ptr) {
      ptr->add_ref();
    }
  }

  intrusive_ptr(const intrusive_ptr &other) : intrusive_ptr{other.ptr, true} {}

  intrusive_ptr(intrusive_ptr &&other) noexcept : ptr{other.ptr} { other.ptr = nullptr; }

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  intrusive_ptr(const intrusive_ptr<Derived> &other) : intrusive_ptr{other.ptr, true} {}

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  intrusive_ptr(intrusive_ptr<Derived> &&other) noexcept : ptr{other.ptr} { other.ptr = nullptr; }

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  intrusive_ptr &operator=(const intrusive_ptr<Derived> &other) {
    *this = intrusive_ptr<T>(other);
    return *this;
  }

  template<class Derived, class = std::enable_if_t<std::is_base_of<T, Derived>{}>>
  intrusive_ptr &operator=(intrusive_ptr<Derived> &&other) {
    *this = intrusive_ptr<T>{std::move(other)};
    return *this;
  }

  intrusive_ptr &operator=(intrusive_ptr other) noexcept {
    std::swap(ptr, other.ptr);
    return *this;
  }

  ~intrusive_ptr() noexcept {
    if (ptr) {
      ptr->release();
    }
  }

  explicit operator bool() const {
    return ptr != nullptr;
  }

  bool unique() const {
    return !ptr || ptr->get_refcnt() == 1;
  }

  T *get() const {
    return ptr;
  }

  template<class Derived>
  vk::intrusive_ptr<Derived> try_as() const {
    return vk::intrusive_ptr<Derived>{dynamic_cast<Derived *>(get())};
  }

  bool operator==(const intrusive_ptr<T> &other) const {
    return ptr == other.ptr;
  }

  bool operator!=(const intrusive_ptr<T> &other) const {
    return !(*this == other);
  }

  friend bool operator==(const intrusive_ptr<T> &lhs, const T *rhs) {
    return lhs.ptr == rhs;
  }

  friend bool operator!=(const intrusive_ptr<T> &lhs, const T *rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const T *lhs, const intrusive_ptr<T> &rhs) {
    return rhs == lhs;
  }

  friend bool operator!=(const T *lhs, const intrusive_ptr<T> &rhs) {
    return rhs != lhs;
  }

  void reset() {
    *this = {};
  }

  T *operator->() const noexcept { return ptr; }

  T &operator*() const noexcept {
    assert(ptr);
    return *ptr;
  }

  void swap(intrusive_ptr &other) {
    std::swap(ptr, other.ptr);
  }

  template<class Derived>
  friend class intrusive_ptr;

private:
  T *ptr{nullptr};
};

template <class T, class... Args>
intrusive_ptr<T> make_intrusive(Args &&... args) {
  return intrusive_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class Derived, class Refcnt>
class refcountable {
public:
  void add_ref() {
    ++refcnt;
  }

  size_t get_refcnt() {
    return refcnt;
  }

  void release() {
    if (--refcnt == 0) {
      delete static_cast<Derived *>(this);
    }
  }

protected:
  Refcnt refcnt{};
};

template <class T>
using thread_safe_refcnt = refcountable<T, std::atomic<size_t>>;
template <class T>
using thread_unsafe_refcnt = refcountable<T, size_t>;

template<class Derived, class Base>
vk::intrusive_ptr<Derived> dynamic_pointer_cast(const vk::intrusive_ptr<Base> &base) {
  return base.template try_as<Derived>();
}

} // namespace vk
#endif // ENGINE_INTRUSIVE_PTR_H
