// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>
#include <random>

#include "common/mixin/not_copyable.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/time/util.h"

struct RandomInstanceState final : private vk::not_copyable {
  std::mt19937_64 mt_gen;
  int32_t lcg1;
  int32_t lcg2;

  RandomInstanceState() noexcept {
    uint64_t seed{};
    k2::os_rnd(sizeof(seed), std::addressof(seed));
    mt_gen = std::mt19937_64{seed};

    // TODO may be replace this with k2::os_rnd ??????
    auto [sec, susec]{system_seconds_and_micros()};
    lcg1 = static_cast<int32_t>(sec.count()) ^ (static_cast<int32_t>(susec.count()) << 11);
    auto [_, susec2]{system_seconds_and_micros()};
    // TODO k2::getpid() will return same value on different instance states,
    // so we really should replace it with k2::os_rnd
    lcg2 = static_cast<int32_t>(k2::getpid()) ^ (static_cast<int32_t>(susec2.count()) << 11);
  }

  static RandomInstanceState& get() noexcept;
};
