// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/type_traits/list_of_types.h"
#include "server/pmemcached-binlog-interface.h"

#include "runtime/kphp_core.h"
#include "runtime/memcache.h"

namespace impl_ {

template<lev_type_t EV_MAGIC>
struct confdata_single_event {
  // use enum as static constexpr variables would require a cpp file
  enum : lev_type_t { MAGIC = EV_MAGIC };
};

template<lev_type_t EV_MAGIC_BASE, lev_type_t EV_MAGIC_MASK>
struct confdata_range_event {
  // use enum as static constexpr variables would require a cpp file
  enum : lev_type_t { MAGIC_BASE = EV_MAGIC_BASE, MAGIC_MASK = EV_MAGIC_MASK };
};

template<class BASE, int STORE_OPERATION>
using select_store_evenet = impl_::confdata_single_event<STORE_OPERATION + std::is_same<BASE, lev_pmemcached_store>{} * LEV_PMEMCACHED_STORE
                                                         + std::is_same<BASE, lev_pmemcached_store_forever>{} * LEV_PMEMCACHED_STORE_FOREVER>;

template<class BASE, class EV, bool USE_TERMINATED_NULL = true>
struct confdata_event : BASE, EV {
public:
  int get_extra_bytes() const noexcept {
    return get_extra_bytes_impl<BASE>(nullptr);
  }

private:
  template<class B>
  int get_extra_bytes_impl(decltype(std::declval<B>().data_len) *) const noexcept {
    return BASE::key_len + BASE::data_len + USE_TERMINATED_NULL + static_cast<int>(offsetof(BASE, data) - sizeof(BASE));
  }

  template<class B>
  int get_extra_bytes_impl(...) const noexcept {
    return BASE::key_len + USE_TERMINATED_NULL + static_cast<int>(offsetof(BASE, key) - sizeof(BASE));
  }
};

} // namespace impl_

using lev_confdata_delete = impl_::confdata_event<lev_pmemcached_delete, impl_::confdata_single_event<LEV_PMEMCACHED_DELETE>>;

using lev_confdata_get = impl_::confdata_event<lev_pmemcached_get, impl_::confdata_single_event<LEV_PMEMCACHED_GET>>;

using lev_confdata_touch = impl_::confdata_event<lev_pmemcached_touch, impl_::confdata_single_event<LEV_PMEMCACHED_TOUCH>>;

using lev_confdata_incr = impl_::confdata_event<lev_pmemcached_incr, impl_::confdata_single_event<LEV_PMEMCACHED_INCR>>;

using lev_confdata_incr_tiny_range = impl_::confdata_event<lev_pmemcached_incr_tiny, impl_::confdata_range_event<LEV_PMEMCACHED_INCR_TINY, 255>>;

using lev_confdata_append = impl_::confdata_event<lev_pmemcached_append, impl_::confdata_single_event<LEV_PMEMCACHED_APPEND>, false>;

template<class BASE, int STORE_OPERATION>
struct lev_confdata_store_wrapper : impl_::confdata_event<BASE, impl_::select_store_evenet<BASE, STORE_OPERATION>> {
public:
  static_assert(vk::is_type_in_list<BASE, index_entry, lev_pmemcached_store, lev_pmemcached_store_forever>{}, "unexpected type");

  mixed get_value_as_var() const noexcept {
    const char *mem = BASE::data + BASE::key_len;
    const int size = BASE::data_len;
    return size <= 0 ? mixed{string{}} : mc_get_value(mem, size, get_flags());
  }

  size_t get_data_size() const noexcept {
    return BASE::data_len;
  }

  vk::string_view get_value_as_string() const noexcept {
    const char *mem = BASE::data + BASE::key_len;
    const int size = BASE::data_len;
    return size <= 0 ? vk::string_view{} : vk::string_view{mem, static_cast<size_t>(size)};
  }

  short get_flags() const noexcept {
    return get_flags_impl<BASE>(nullptr);
  }
  int get_delay() const noexcept {
    return get_delay_impl<BASE>(nullptr);
  }

private:
  template<class B>
  short get_flags_impl(decltype(std::declval<B>().flags) *) const noexcept {
    return BASE::flags;
  }

  template<class B>
  short get_flags_impl(...) const noexcept {
    return 0;
  }

  template<class B>
  int get_delay_impl(decltype(std::declval<B>().delay) *) const noexcept {
    return BASE::delay;
  }

  template<class B>
  int get_delay_impl(...) const noexcept {
    return -1;
  }
};
