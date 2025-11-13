// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/time/datetime-interface.h"

struct C$DateTime : public C$DateTimeInterface, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  const char* get_class() const noexcept override {
    return R"(DateTime)";
  }

  int32_t get_hash() const noexcept override {
    std::string_view name_view{C$DateTime::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  ~C$DateTime() override;
};

constexpr std::string_view NOW{"now"};
