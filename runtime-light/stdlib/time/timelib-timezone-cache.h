// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/utils/logs.h"

namespace kphp::timelib {

class timezone_cache final {
  kphp::stl::unordered_set<timelib_tzinfo*, kphp::memory::script_allocator, decltype([](const timelib_tzinfo* tzinfo) noexcept {
                             return tzinfo != nullptr ? std::hash<std::string_view>{}({tzinfo->name}) : 0;
                           }),
                           decltype([](const timelib_tzinfo* lhs, const timelib_tzinfo* rhs) noexcept {
                             if (lhs == nullptr || rhs == nullptr) [[unlikely]] {
                               return lhs == rhs;
                             }
                             return std::strcmp(lhs->name, rhs->name) == 0;
                           })>
      m_cache;

public:
  timelib_tzinfo* get(std::string_view tzname) const noexcept {
    timelib_tzinfo tmp{.name = const_cast<char*>(tzname.data())};
    auto it{m_cache.find(std::addressof(tmp))};
    return it != m_cache.end() ? *it : nullptr;
  }

  void put(timelib_tzinfo* tzinfo) noexcept {
    if (tzinfo == nullptr) [[unlikely]] {
      kphp::log::warning("nullptr timelib_tzinfo detected in timezone_cache");
      return;
    }
    m_cache.emplace(tzinfo);
  }

  timezone_cache() noexcept = default;

  timezone_cache(std::initializer_list<std::string_view> tzs) noexcept {
    kphp::memory::libc_alloc_guard _{};
    std::ranges::for_each(tzs, [this](std::string_view tz) noexcept {
      int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
      auto* tzinfo{timelib_parse_tzfile(tz.data(), timelib_builtin_db(), std::addressof(errc))};
      if (tzinfo == nullptr) [[unlikely]] {
        kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", tz, timelib_get_error_message(errc));
        return;
      }
      m_cache.emplace(tzinfo);
    });
  }

  timezone_cache(timezone_cache&& other) noexcept
      : m_cache(std::move(other.m_cache)) {}

  timezone_cache& operator=(timezone_cache&& other) noexcept {
    if (this != std::addressof(other)) {
      m_cache = std::move(other.m_cache);
    }
    return *this;
  }

  timezone_cache(const timezone_cache&) = delete;
  timezone_cache& operator=(const timezone_cache&) = delete;

  ~timezone_cache() {
    kphp::memory::libc_alloc_guard _{};
    std::ranges::for_each(m_cache, [](timelib_tzinfo* tzinfo) noexcept { timelib_tzinfo_dtor(tzinfo); });
  }
};

} // namespace kphp::timelib
