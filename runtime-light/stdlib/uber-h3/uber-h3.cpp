// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>

#include "h3/h3api.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/uber-h3/uber-h3.h"
#include "runtime-light/allocator/allocator.h"

Optional<array<int64_t>> f$UberH3$$kRing(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k{uber_h3_impl_::check_k_param(k)};
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count{maxKringSize(checked_k)};
  auto neighbor_indexes{uber_h3_impl_::make_zeros_vector<int64_t>(neighbors_count)};
  if (neighbors_count) {
    // kRing() uses malloc
    kphp::memory::libc_alloc_guard _{};
    kRing(h3_index_origin, checked_k, reinterpret_cast<H3Index*>(std::addressof(neighbor_indexes[0])));
  }
  return std::move(neighbor_indexes);
}

Optional<array<int64_t>> f$UberH3$$compact(const array<int64_t>& h3_indexes) noexcept {
  const array<int64_t> h3_set{uber_h3_impl_::indexes2vector(h3_indexes)};
  auto compacted_h3_set{uber_h3_impl_::make_zeros_vector<int64_t>(h3_set.count())};
  if (!compacted_h3_set.empty()) {
    // compact() uses malloc
    kphp::memory::libc_alloc_guard _{};
    if (unlikely(compact(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), reinterpret_cast<H3Index*>(std::addressof(compacted_h3_set[0])),
                         static_cast<int32_t>(h3_indexes.count())))) {
      return false;
    }
  }
  int64_t left{compacted_h3_set.count()};
  while (left && !compacted_h3_set[--left]) {
    compacted_h3_set.pop();
  }
  return std::move(compacted_h3_set);
}

Optional<array<int64_t>> f$UberH3$$polyfill(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                            int64_t resolution) noexcept {
  const int32_t checked_resolution{uber_h3_impl_::check_resolution_param(resolution)};
  if (unlikely(checked_resolution != resolution)) {
    return false;
  }

  uber_h3_impl_::GeoPolygonOwner polygon_owner{polygon_boundary, holes};
  const int32_t max_size{maxPolyfillSize(std::addressof(polygon_owner.getPolygon()), checked_resolution)};
  if (max_size < 0) {
    return false;
  }
  auto hexagon_indexes{uber_h3_impl_::make_zeros_vector<int64_t>(max_size)};
  if (!hexagon_indexes.empty()) {
    // polyfill() uses malloc
    kphp::memory::libc_alloc_guard _{};
    polyfill(std::addressof(polygon_owner.getPolygon()), checked_resolution, reinterpret_cast<H3Index*>(&hexagon_indexes[0]));
  }
  int64_t indexes_count{0};
  for (const auto& element : hexagon_indexes) {
    indexes_count += element.get_value() ? 1 : 0;
  }
  array<int64_t> result_array{array_size{indexes_count, true}};
  for (const auto& element : hexagon_indexes) {
    if (auto h3_index{element.get_value()}) {
      result_array.emplace_back(h3_index);
    }
  }

  return std::move(result_array);
}
