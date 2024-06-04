#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

namespace __runtime_core {
template<class T, typename Allocator>
class_instance<T, Allocator> &class_instance<T, Allocator>::operator=(const Optional<bool> &null) noexcept {
  php_assert(null.value_state() == OptionalState::null_value);
  o.reset();
  return *this;
}

template<class T, typename Allocator>
class_instance<T, Allocator> class_instance<T, Allocator>::clone_impl(std::true_type /*is empty*/) const {
  return class_instance{}.empty_alloc();
}

template<class T, typename Allocator>
class_instance<T, Allocator> class_instance<T, Allocator>::clone_impl(std::false_type /*is empty*/) const {
  class_instance res;
  if (o) {
    res.alloc(*o);
    res.o->set_refcnt(1);
  }
  return res;
}

template<class T, typename Allocator>
class_instance<T, Allocator> class_instance<T, Allocator>::clone() const {
  return clone_impl(std::is_empty<T>{});
}

template<class T, typename Allocator>
template<class... Args>
class_instance<T, Allocator> class_instance<T, Allocator>::alloc(Args &&...args) {
  static_assert(!std::is_empty<T>{}, "class T may not be empty");
  php_assert(!o);
  new (&o) vk::intrusive_ptr<T>(new T{std::forward<Args>(args)...});
  return *this;
}

template<class T, typename Allocator>
inline class_instance<T, Allocator> class_instance<T, Allocator>::empty_alloc() {
  static_assert(std::is_empty<T>{}, "class T must be empty");
  uint64_t obj = KphpCoreContext::current().empty_alloc_counter++;
  new (&o) vk::intrusive_ptr<T>(reinterpret_cast<T *>(obj));
  return *this;
}

template<class T, typename Allocator>
T *class_instance<T, Allocator>::operator->() {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return get();
};

template<class T, typename Allocator>
T *class_instance<T, Allocator>::operator->() const {
  if (unlikely(!o)) {
    warn_on_access_null();
  }

  return get();
};

template<class T, typename Allocator>
T *class_instance<T, Allocator>::get() const {
  static_assert(!std::is_empty<T>{}, "class T may not be empty");
  return o.get();
}

template<class T, typename Allocator>
void class_instance<T, Allocator>::warn_on_access_null() const {
  php_warning("Trying to access property of null object");
  const_cast<class_instance *>(this)->alloc();
}

template<class T, typename Allocator>
void class_instance<T, Allocator>::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
  php_assert(o->get_refcnt() == 1);
  o->set_refcnt(static_cast<uint32_t>(static_cast<int32_t>(ref_cnt_value)));
}

template<class T, typename Allocator>
bool class_instance<T, Allocator>::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
  return static_cast<int32_t>(o->get_refcnt()) == ref_cnt_value;
}

template<class T, typename Allocator>
void class_instance<T, Allocator>::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  if (o) {
    php_assert(static_cast<int32_t>(o->get_refcnt()) == expected_ref_cnt);
    o->set_refcnt(1);
    destroy();
  }
}
}
