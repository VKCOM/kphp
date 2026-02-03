// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstring>
#include <expected>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-types.h"

namespace kphp::timelib {

class timezone_cache final {
  struct tzinfo_hash_t : std::hash<std::string_view> {
    using is_transparent = void;
    using std::hash<std::string_view>::operator();

    size_t operator()(const kphp::timelib::tzinfo_holder& tzinfo) const noexcept {
      kphp::log::assertion(tzinfo != nullptr);
      return tzinfo->name != nullptr ? (*this)(tzinfo->name) : 0;
    }
  };

  struct tzinfo_comparator_t : std::equal_to<std::string_view> {
    using is_transparent = void;
    using std::equal_to<std::string_view>::operator();

    bool operator()(const kphp::timelib::tzinfo_holder& lhs, const auto& rhs) const noexcept {
      if (lhs == nullptr || lhs->name == nullptr) [[unlikely]] {
        return false;
      }
      return (*this)(lhs->name, rhs);
    }
    bool operator()(std::string_view lhs, const kphp::timelib::tzinfo_holder& rhs) const noexcept {
      if (rhs == nullptr || rhs->name == nullptr) [[unlikely]] {
        return false;
      }
      return (*this)(lhs, rhs->name);
    }
  };

  kphp::stl::unordered_set<kphp::timelib::tzinfo_holder, kphp::memory::script_allocator, tzinfo_hash_t, tzinfo_comparator_t> m_cache;

public:
  std::optional<std::reference_wrapper<const kphp::timelib::tzinfo_holder>> get(std::string_view tzname) const noexcept {
    auto it{m_cache.find(tzname)};
    return it != m_cache.end() ? std::make_optional(std::cref(*it)) : std::nullopt;
  }

  std::optional<std::reference_wrapper<const kphp::timelib::tzinfo_holder>> put(kphp::timelib::tzinfo_holder&& tzinfo) noexcept {
    return *m_cache.emplace(std::move(tzinfo)).first;
  }

  timezone_cache() noexcept = default;

  timezone_cache(std::initializer_list<std::string_view> tzs) noexcept {
    std::ranges::for_each(tzs, [this](std::string_view tz) noexcept {
      int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
      kphp::timelib::tzinfo_holder tzinfo{timelib_parse_tzfile(tz.data(), timelib_builtin_db(), std::addressof(errc)),
                                          kphp::timelib::details::tzinfo_destructor};
      if (tzinfo == nullptr || tzinfo->name == nullptr) [[unlikely]] {
        kphp::log::warning("failed to load {} tzinfo on timezone_cache initialization", tz);
        return;
      }
      put(std::move(tzinfo));
    });
  }

  timezone_cache(timezone_cache&& other) noexcept = default;
  timezone_cache& operator=(timezone_cache&& other) noexcept = default;

  timezone_cache(const timezone_cache&) = delete;
  timezone_cache& operator=(const timezone_cache&) = delete;

  ~timezone_cache() = default;
};

} // namespace kphp::timelib
