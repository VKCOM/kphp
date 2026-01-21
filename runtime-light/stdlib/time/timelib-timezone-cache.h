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
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-types.h"

namespace kphp::timelib {

class timezone_cache final {
  struct tzinfo_hash_t : std::hash<std::string_view> {
    using is_transparent = void;
    using std::hash<std::string_view>::operator();

    size_t operator()(const kphp::timelib::tzinfo& tzinfo) const noexcept {
      kphp::log::assertion(tzinfo != nullptr);
      return tzinfo->name != nullptr ? (*this)(tzinfo->name) : 0;
    }
  };

  struct tzinfo_comparator_t : std::equal_to<std::string_view> {
    using is_transparent = void;
    using std::equal_to<std::string_view>::operator();

    bool operator()(const kphp::timelib::tzinfo& lhs, const auto& rhs) const noexcept {
      if (lhs == nullptr || lhs->name == nullptr) [[unlikely]] {
        return false;
      }
      return (*this)(lhs->name, rhs);
    }
    bool operator()(std::string_view lhs, const kphp::timelib::tzinfo& rhs) const noexcept {
      if (rhs == nullptr || rhs->name == nullptr) [[unlikely]] {
        return false;
      }
      return (*this)(lhs, rhs->name);
    }
  };

  kphp::stl::unordered_set<kphp::timelib::tzinfo, kphp::memory::script_allocator, tzinfo_hash_t, tzinfo_comparator_t> m_cache;

public:
  std::optional<std::reference_wrapper<const kphp::timelib::tzinfo>> get(std::string_view tzname) const noexcept {
    auto it{m_cache.find(tzname)};
    return it != m_cache.end() ? std::make_optional(std::cref(*it)) : std::nullopt;
  }

  std::optional<std::reference_wrapper<const kphp::timelib::tzinfo>> put(kphp::timelib::tzinfo&& tzinfo) noexcept {
    return *m_cache.emplace(std::move(tzinfo)).first;
  }

  std::expected<std::reference_wrapper<const kphp::timelib::tzinfo>, int32_t> make(std::string_view tz, const timelib_tzdb* tzdb) noexcept {
    int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
    kphp::timelib::tzinfo tzinfo{timelib_parse_tzfile(tz.data(), tzdb, std::addressof(errc)), kphp::timelib::details::tzinfo_destructor};
    if (tzinfo == nullptr || tzinfo->name == nullptr) [[unlikely]] {
      return std::unexpected{errc};
    }
    return *m_cache.emplace(std::move(tzinfo)).first;
  }

  void clear() noexcept {
    kphp::memory::libc_alloc_guard{}, m_cache.clear();
  }

  timezone_cache() noexcept = default;

  timezone_cache(std::initializer_list<std::string_view> tzs) noexcept {
    kphp::memory::libc_alloc_guard _{};
    std::ranges::for_each(tzs, [this](std::string_view tz) noexcept {
      int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
      kphp::timelib::tzinfo tzinfo{timelib_parse_tzfile(tz.data(), timelib_builtin_db(), std::addressof(errc)), kphp::timelib::details::tzinfo_destructor};
      if (tzinfo == nullptr || tzinfo->name == nullptr) [[unlikely]] {
        return;
      }
      put(std::move(tzinfo));
    });
  }

  timezone_cache(timezone_cache&& other) noexcept
      : m_cache(std::move(other.m_cache)) {}

  timezone_cache& operator=(timezone_cache&& other) noexcept {
    if (this != std::addressof(other)) {
      clear();
      m_cache = std::move(other.m_cache);
    }
    return *this;
  }

  timezone_cache(const timezone_cache&) = delete;
  timezone_cache& operator=(const timezone_cache&) = delete;

  ~timezone_cache() {
    if (!m_cache.empty()) {
      clear();
    }
  }
};

} // namespace kphp::timelib
