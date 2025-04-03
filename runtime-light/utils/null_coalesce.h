// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "runtime-common/core/include.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"

namespace null_coalesce_impl_ {

template<class ReturnType, class FallbackType>
auto perform_fallback_impl(FallbackType &&lambda_fallback, std::enable_if_t<bool(sizeof((std::declval<FallbackType>()(), 0)))> *) noexcept {
  return ReturnType(lambda_fallback());
}

template<class ReturnType, class FallbackType>
requires(kphp::coro::is_async_function_v<FallbackType>) kphp::coro::task<ReturnType> perform_fallback_impl(
  FallbackType &&lambda_fallback, std::enable_if_t<bool(sizeof((std::declval<FallbackType>()(), 0)))> *) noexcept {
  co_return ReturnType(co_await lambda_fallback());
}

template<class ReturnType, class FallbackType>
ReturnType perform_fallback_impl(FallbackType &&value_fallback, ...) noexcept {
  return ReturnType(std::forward<FallbackType>(value_fallback));
}

template<class ReturnType, class FallbackType>
requires(kphp::coro::is_async_function_v<FallbackType>) kphp::coro::task<ReturnType> perform_fallback_impl(FallbackType &&value_fallback) noexcept {
  co_return ReturnType(std::forward<FallbackType>(value_fallback));
}

template<class ReturnType, class FallbackType>
ReturnType perform_fallback(FallbackType &&value_fallback) noexcept {
  return perform_fallback_impl<ReturnType>(std::forward<FallbackType>(value_fallback), nullptr);
}

template<class ReturnType, class FallbackType>
requires(kphp::coro::is_async_function_v<FallbackType>) kphp::coro::task<ReturnType> perform_fallback(FallbackType &&value_fallback) noexcept {
  co_return co_await perform_fallback_impl<ReturnType>(std::forward<FallbackType>(value_fallback), nullptr);
}

} // namespace null_coalesce_impl_

template<typename ResultType>
struct NullCoalesce {
public:
  template<class ValueType>
  explicit NullCoalesce(ValueType &&value) noexcept {
    set_result(std::forward<ValueType>(value));
  }

  template<class ValueType, class KeyType, class = vk::enable_if_constructible<mixed, KeyType>>
  NullCoalesce(const array<ValueType> &arr, const KeyType &key) noexcept {
    if (auto *value = arr.find_value(key)) {
      set_result(*value);
    }
  }

  template<class ValueType>
  NullCoalesce(const array<ValueType> &arr, const string &string_key, int64_t precomuted_hash) noexcept {
    if (auto *value = arr.find_value(string_key, precomuted_hash)) {
      set_result(*value);
    }
  }

  template<class KeyType, class = vk::enable_if_constructible<mixed, KeyType>>
  NullCoalesce(const string &str, const KeyType &key) noexcept {
    set_result(str, key);
  }

  template<class KeyType, class = vk::enable_if_constructible<mixed, KeyType>>
  NullCoalesce(const mixed &v, const KeyType &key) noexcept {
    if (v.is_string()) {
      set_result(v.as_string(), key);
    } else {
      set_result(v.get_value(key));
    }
  }

  NullCoalesce(const mixed &v, const string &string_key, int64_t precomuted_hash) noexcept {
    if (v.is_string()) {
      set_result(v.as_string(), string_key);
    } else {
      set_result(v.get_value(string_key, precomuted_hash));
    }
  }

  template<class FallbackType>
  ResultType finalize(FallbackType &&fallback) noexcept {
    return result_ ? std::move(*result_) : null_coalesce_impl_::perform_fallback<ResultType>(std::forward<FallbackType>(fallback));
  }

  template<class FallbackType>
  requires(kphp::coro::is_async_function_v<FallbackType>) kphp::coro::task<ResultType> finalize(FallbackType &&fallback) noexcept {
    co_return result_ ? std::move(*result_) : co_await null_coalesce_impl_::perform_fallback<ResultType>(std::forward<FallbackType>(fallback));
  }

  ~NullCoalesce() noexcept {
    if (result_) {
      result_->~ResultType();
    }
  }

private:
  template<class ValueType>
  using IsOptionalValueAndNonOptionalReturn =
    std::integral_constant<bool, (is_optional<std::decay_t<ValueType>>{} && !is_optional<std::decay_t<ResultType>>{})>;

  template<class ValueType>
  std::enable_if_t<!IsOptionalValueAndNonOptionalReturn<ValueType>{}> set_result(ValueType &&value) noexcept {
    if (!f$is_null(value)) {
      result_ = new (&result_storage_) ResultType(std::forward<ValueType>(value));
    }
  }

  template<class OptionalValueType>
  std::enable_if_t<IsOptionalValueAndNonOptionalReturn<OptionalValueType>{}> set_result(OptionalValueType &&value) noexcept {
    // keep in mind that ReturnType is not an optional here!
    using result_can_be_false = vk::is_type_in_list<ResultType, bool, mixed>;
    using is_bool_inside = std::is_same<typename std::decay_t<OptionalValueType>::InnerType, bool>;
    using false_cast_immposible = std::integral_constant<bool, is_bool_inside{} && !result_can_be_false{}>;

    php_assert((result_can_be_false{} || !value.is_false()));
    set_result_resolve_or_false(false_cast_immposible{}, std::forward<OptionalValueType>(value));
  }

  template<class KeyType>
  void set_result(const string &str, const KeyType &key) noexcept {
    string value = str.get_value(key);
    if (!value.empty()) {
      result_ = new (&result_storage_) ResultType(std::move(value));
    }
  }

  template<class ValueType>
  void set_result_resolve_or_false(std::true_type, const Optional<ValueType> &value) noexcept {
    php_assert(value.value_state() == OptionalState::null_value);
  }

  template<class ValueType>
  void set_result_resolve_or_false(std::false_type, const Optional<ValueType> &value) noexcept {
    if (!f$is_null(value)) {
      result_ = new (&result_storage_) ResultType(value.val());
    }
  }

  template<class ValueType>
  void set_result_resolve_or_false(std::false_type, Optional<ValueType> &&value) noexcept {
    if (!f$is_null(value)) {
      result_ = new (&result_storage_) ResultType(std::move(value.val()));
      value = {};
    }
  }

  ResultType *result_{nullptr};
  std::aligned_storage_t<sizeof(ResultType), alignof(ResultType)> result_storage_;
};
