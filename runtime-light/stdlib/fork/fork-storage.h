// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/small-object-storage.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::forks::details {

class storage {
  using data_type = small_object_storage<sizeof(mixed)>;

  template<typename From, typename To, typename Convertible = std::is_convertible<From, To>::type>
  struct loader_impl;

  template<typename From, typename To>
  requires(!std::same_as<From, int32_t> && !std::same_as<To, int32_t>)
  struct loader_impl<From, To, std::true_type> {
    static To loader_function(data_type& storage) noexcept {
      From* data{storage.get<From>()};
      To result(std::move(*data));
      storage.destroy<From>();
      return result;
    }
  };

  template<typename From, typename To>
  requires(!std::same_as<From, int32_t> && !std::same_as<To, int32_t>)
  struct loader_impl<From, To, std::false_type> {
    static To loader_function([[maybe_unused]] data_type& storage) noexcept {
      kphp::log::assertion(false);
      return To{};
    }
  };

  std::optional<int32_t> m_opt_tag;
  data_type m_data;

public:
  template<typename T>
  requires(!std::same_as<T, int32_t>)
  struct tagger {
    static auto get_tag() noexcept -> int32_t;
  };

  template<typename T>
  requires(!std::same_as<T, int32_t>)
  struct loader {
    using loader_function_type = T (*)(data_type&);

    static auto get_loader(int32_t tag) noexcept -> loader_function_type;
  };

  // `std::type_identity_t` is used to disable type deduction for `store` function.
  // It should be called with exactly the same type parameter as `load` function.
  // So, to prevent a bug we decided to forbid type deduction here.
  template<typename T>
  requires(!std::same_as<T, int32_t> && !std::same_as<T, void>)
  auto store(std::type_identity_t<T> val) noexcept -> void {
    kphp::log::assertion(!m_opt_tag.has_value());
    m_opt_tag.emplace(tagger<T>::get_tag());
    m_data.emplace<T>(std::move(val));
  }

  auto store() noexcept -> void;

  template<typename T>
  requires(!std::same_as<T, int32_t>)
  auto load() noexcept -> T {
    kphp::log::assertion(m_opt_tag.has_value());
    const auto tag{*m_opt_tag};
    m_opt_tag = std::nullopt;
    if constexpr (!std::same_as<T, void>) {
      return loader<T>::get_loader(tag)(m_data);
    }
  }
};

} // namespace kphp::forks::details
