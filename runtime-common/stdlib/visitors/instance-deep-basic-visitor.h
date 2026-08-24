// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "common/type_traits/list_of_types.h"
#include "runtime-common/core/runtime-core.h"

namespace kphp::visitors {
// high bit of the per-instance refcnt info word used by the legacy counting/destroy visitors:
// the lower bits store the real reference count, the top bit marks the instance as visited
inline constexpr uint32_t VISITED_INSTANCE_MASK{0x80000000};

// CRTP base for visitors that traverse an instance graph via compiler-generated accept() methods:
// every field is dispatched to Child::process; a false result from any field is accumulated into is_ok()
template<typename Child>
class instance_deep_basic_visitor : vk::not_copyable {
public:
  template<typename T>
  void operator()(const char* /*unused*/, T&& value) noexcept {
    const bool is_ok{child_.process(std::forward<T>(value))};
    is_ok_ = is_ok_ && is_ok;
  }

  template<typename T>
  bool process(T& /*unused*/) noexcept {
    return true;
  }

  template<typename T>
  bool process(Optional<T>& value) noexcept {
    return !value.has_value() || child_.process(value.val());
  }

  template<typename I>
  bool process(class_instance<I>& instance) noexcept {
    if (!instance.is_null()) {
      instance.get()->accept(child_);
      return child_.is_ok();
    }
    return true;
  }

  template<typename... Args>
  bool process(std::tuple<Args...>& value) noexcept {
    return process_tuple(value);
  }

  template<size_t... Is, typename... T>
  bool process(shape<std::index_sequence<Is...>, T...>& value) noexcept {
    const bool child_res[]{child_.process(value.template get<Is>())...};
    return std::all_of(std::begin(child_res), std::end(child_res), [](bool r) noexcept { return r; });
  }

  bool process(mixed& value) noexcept {
    if (value.is_string()) {
      return child_.process(value.as_string());
    } else if (value.is_array()) {
      return child_.process(value.as_array());
    }
    return true;
  }

  bool is_ok() const noexcept {
    return is_ok_;
  }

  ExtraRefCnt get_memory_ref_cnt() const noexcept {
    return memory_ref_cnt_;
  }

protected:
  template<class T>
  static constexpr bool is_primitive{vk::is_type_in_list<T, int64_t, double, bool, Optional<int64_t>, Optional<double>, Optional<bool>>::value};

  explicit instance_deep_basic_visitor(Child& child, ExtraRefCnt memory_ref_cnt = ExtraRefCnt::extra_ref_cnt_value(0)) noexcept
      : memory_ref_cnt_{memory_ref_cnt},
        child_{child} {}

  template<typename Iterator>
  bool process_range(Iterator first, Iterator last) noexcept {
    bool res{true};
    for (; first != last; ++first) {
      if (!child_.process(first.get_value())) {
        res = false;
      }
      if (first.is_string_key() && !child_.process(first.get_string_key())) {
        res = false;
      }
    }
    return res;
  }

private:
  template<size_t Index = 0, typename... Args>
  std::enable_if_t<Index != sizeof...(Args), bool> process_tuple(std::tuple<Args...>& value) noexcept {
    bool res = child_.process(std::get<Index>(value));
    return process_tuple<Index + 1>(value) && res;
  }

  template<size_t Index = 0, typename... Args>
  std::enable_if_t<Index == sizeof...(Args), bool> process_tuple(std::tuple<Args...>& /*unused*/) noexcept {
    return true;
  }

  bool is_ok_{true};
  const ExtraRefCnt memory_ref_cnt_{ExtraRefCnt::extra_ref_cnt_value(0)};
  Child& child_;
};

} // namespace kphp::visitors
