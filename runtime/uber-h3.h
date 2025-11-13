// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <tuple>

#include "runtime-common/core/runtime-core.h"

Optional<array<int64_t>> f$UberH3$$kRing(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$compact(const array<int64_t>& h3_indexes) noexcept;
Optional<array<int64_t>> f$UberH3$$polyfill(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                            int64_t resolution) noexcept;
