// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>
#include <random>

#include "common/mixin/not_copyable.h"
#include "runtime-light/k2-platform/k2-api.h"

struct RandomInstanceState final : private vk::not_copyable {
  std::mt19937_64 mt_gen;

  RandomInstanceState() noexcept {
    uint64_t seed{};
    k2::os_rnd(sizeof(seed), std::addressof(seed));
    mt_gen = std::mt19937_64{seed};
  }

  static RandomInstanceState& get() noexcept;
};
