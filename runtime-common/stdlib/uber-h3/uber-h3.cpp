// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/uber-h3/uber-h3.h"

#include <array>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "h3/h3api.h"

#include "runtime-common/core/runtime-core.h"

int64_t f$UberH3$$geoToH3(double latitude, double longitude, int64_t resolution) noexcept {
  static_assert(std::is_same<H3Index, uint64_t>{}, "H3Index expected to be uint64_t");
  const int32_t checked_resolution = uber_h3_impl_::check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  const auto geo_cord = uber_h3_impl_::deg2coord(std::make_tuple(latitude, longitude));
  return geoToH3(std::addressof(geo_cord), checked_resolution);
}

std::tuple<double, double> f$UberH3$$h3ToGeo(int64_t h3_index) noexcept {
  GeoCoord geo_coord{};
  h3ToGeo(h3_index, std::addressof(geo_coord));
  return uber_h3_impl_::coord2deg(geo_coord);
}

array<std::tuple<double, double>> f$UberH3$$h3ToGeoBoundary(int64_t h3_index) noexcept {
  GeoBoundary boundary;
  h3ToGeoBoundary(h3_index, std::addressof(boundary));
  array<std::tuple<double, double>> result{array_size{boundary.numVerts, true}};
  for (int i = 0; i < boundary.numVerts; ++i) {
    result.emplace_back(uber_h3_impl_::coord2deg(boundary.verts[i]));
  }
  return result;
}

int64_t f$UberH3$$h3GetResolution(int64_t h3_index) noexcept {
  return h3GetResolution(static_cast<H3Index>(h3_index));
}

int64_t f$UberH3$$h3GetBaseCell(int64_t h3_index) noexcept {
  return h3GetBaseCell(static_cast<H3Index>(h3_index));
}

int64_t f$UberH3$$stringToH3(const string& h3_index_str) noexcept {
  return stringToH3(h3_index_str.c_str());
}

string f$UberH3$$h3ToString(int64_t h3_index) noexcept {
  std::array<char, 36> buff{};
  h3ToString(static_cast<H3Index>(h3_index), buff.data(), buff.size());
  return string{buff.data()};
}

bool f$UberH3$$h3IsValid(int64_t h3_index) noexcept {
  return h3IsValid(static_cast<H3Index>(h3_index)) != 0;
}

bool f$UberH3$$h3IsResClassIII(int64_t h3_index) noexcept {
  return h3IsResClassIII(static_cast<H3Index>(h3_index)) != 0;
}

bool f$UberH3$$h3IsPentagon(int64_t h3_index) noexcept {
  return h3IsPentagon(static_cast<H3Index>(h3_index)) != 0;
}

array<int64_t> f$UberH3$$h3GetFaces(int64_t h3_index) noexcept {
  const int64_t face_count = maxFaceCount(static_cast<H3Index>(h3_index));
  auto int32_result = uber_h3_impl_::make_zeros_vector<int32_t>(face_count);
  if (face_count) {
    h3GetFaces(h3_index, std::addressof(int32_result[0]));
  }
  return array<int64_t>::convert_from(int32_result);
}

int64_t f$UberH3$$maxFaceCount(int64_t h3_index) noexcept {
  return maxFaceCount(static_cast<H3Index>(h3_index));
}

int64_t f$UberH3$$maxKringSize(int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  return checked_k != k ? 0 : maxKringSize(checked_k);
}

Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$kRingDistances(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count = maxKringSize(checked_k);
  array<std::tuple<int64_t, int64_t>> result{array_size{neighbors_count, true}};
  if (neighbors_count) {
    auto neighbor_indexes = uber_h3_impl_::make_zeros_vector<H3Index>(neighbors_count);
    auto neighbor_distances = uber_h3_impl_::make_zeros_vector<int32_t>(neighbors_count);
    kRingDistances(h3_index_origin, checked_k, std::addressof(neighbor_indexes[0]), std::addressof(neighbor_distances[0]));
    for (int i = 0; i < neighbors_count; ++i) {
      result.emplace_back(std::make_tuple(neighbor_indexes[i], neighbor_distances[i]));
    }
  }
  return std::move(result);
}

Optional<array<int64_t>> f$UberH3$$hexRange(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto neighbors = uber_h3_impl_::make_zeros_vector<int64_t>(maxKringSize(checked_k));
  if (!neighbors.empty()) {
    if (unlikely(hexRange(h3_index_origin, checked_k, reinterpret_cast<H3Index*>(std::addressof(neighbors[0]))))) {
      return false;
    }
  }
  return std::move(neighbors);
}

Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$hexRangeDistances(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count = maxKringSize(checked_k);
  array<std::tuple<int64_t, int64_t>> result{array_size{neighbors_count, true}};
  if (neighbors_count) {
    auto neighbor_indexes = uber_h3_impl_::make_zeros_vector<H3Index>(neighbors_count);
    auto neighbor_distances = uber_h3_impl_::make_zeros_vector<int32_t>(neighbors_count);
    if (unlikely(hexRangeDistances(h3_index_origin, checked_k, std::addressof(neighbor_indexes[0]), std::addressof(neighbor_distances[0])))) {
      return false;
    }
    for (int i = 0; i < neighbors_count; ++i) {
      result.emplace_back(std::make_tuple(neighbor_indexes[i], neighbor_distances[i]));
    }
  }
  return std::move(result);
}

Optional<array<int64_t>> f$UberH3$$hexRanges(const array<int64_t>& h3_indexes, int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto h3_indexes_set = uber_h3_impl_::indexes2vector(h3_indexes, true);
  auto h3_indexes_result = uber_h3_impl_::make_zeros_vector<int64_t>(maxKringSize(checked_k) * h3_indexes.count());
  if (!h3_indexes_result.empty()) {
    if (unlikely(hexRanges(reinterpret_cast<H3Index*>(std::addressof(h3_indexes_set[0])), static_cast<int32_t>(h3_indexes.count()), checked_k,
                           reinterpret_cast<H3Index*>(std::addressof(h3_indexes_result[0]))))) {
      return false;
    }
  }
  return std::move(h3_indexes_result);
}

Optional<array<int64_t>> f$UberH3$$hexRing(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = uber_h3_impl_::check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto h3_indexes_result = uber_h3_impl_::make_zeros_vector<int64_t>(checked_k ? checked_k * 6 : 1);
  if (!h3_indexes_result.empty()) {
    hexRing(h3_index_origin, checked_k, reinterpret_cast<H3Index*>(std::addressof(h3_indexes_result[0])));
  }
  return std::move(h3_indexes_result);
}

Optional<array<int64_t>> f$UberH3$$h3Line(int64_t h3_index_start, int64_t h3_index_end) noexcept {
  const int64_t size = h3LineSize(static_cast<H3Index>(h3_index_start), static_cast<H3Index>(h3_index_end));
  if (unlikely(size < 0)) {
    return false;
  }

  auto line = uber_h3_impl_::make_zeros_vector<int64_t>(size);
  if (size) {
    if (unlikely(h3Line(static_cast<H3Index>(h3_index_start), static_cast<H3Index>(h3_index_end), reinterpret_cast<H3Index*>(std::addressof(line[0]))))) {
      return false;
    }
  }
  return std::move(line);
}

int64_t f$UberH3$$h3LineSize(int64_t h3_index_start, int64_t h3_index_end) noexcept {
  return h3LineSize(static_cast<H3Index>(h3_index_start), static_cast<H3Index>(h3_index_end));
}

int64_t f$UberH3$$h3Distance(int64_t h3_index_start, int64_t h3_index_end) noexcept {
  return h3Distance(static_cast<H3Index>(h3_index_start), static_cast<H3Index>(h3_index_end));
}

int64_t f$UberH3$$h3ToParent(int64_t h3_index, int64_t parent_resolution) noexcept {
  const int32_t checked_parent_resolution = uber_h3_impl_::check_resolution_param(parent_resolution);
  return checked_parent_resolution != parent_resolution ? 0 : h3ToParent(static_cast<H3Index>(h3_index), checked_parent_resolution);
}

Optional<array<int64_t>> f$UberH3$$h3ToChildren(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = uber_h3_impl_::check_resolution_param(children_resolution);
  if (unlikely(checked_children_resolution != children_resolution)) {
    return false;
  }
  const int64_t children_count = maxH3ToChildrenSize(static_cast<H3Index>(h3_index), checked_children_resolution);
  auto children = uber_h3_impl_::make_zeros_vector<int64_t>(children_count);
  if (children_count) {
    h3ToChildren(static_cast<H3Index>(h3_index), checked_children_resolution, reinterpret_cast<H3Index*>(std::addressof(children[0])));
  }
  return std::move(children);
}

int64_t f$UberH3$$maxH3ToChildrenSize(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = uber_h3_impl_::check_resolution_param(children_resolution);
  return checked_children_resolution != children_resolution ? 0 : maxH3ToChildrenSize(static_cast<H3Index>(h3_index), checked_children_resolution);
}

int64_t f$UberH3$$h3ToCenterChild(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = uber_h3_impl_::check_resolution_param(children_resolution);
  return checked_children_resolution != children_resolution ? 0 : h3ToCenterChild(static_cast<H3Index>(h3_index), checked_children_resolution);
}

Optional<array<int64_t>> f$UberH3$$uncompact(const array<int64_t>& h3_indexes, int64_t resolution) noexcept {
  const int32_t checked_resolution = uber_h3_impl_::check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return false;
  }

  const auto h3_set_size = static_cast<int32_t>(h3_indexes.count());
  const array<int64_t> h3_set = uber_h3_impl_::indexes2vector(h3_indexes);
  const int32_t uncompact_size = maxUncompactSize(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), h3_set_size, checked_resolution);
  if (unlikely(uncompact_size < 0)) {
    return false;
  }
  if (!uncompact_size) {
    return array<int64_t>{};
  }
  auto uncompacted_h3_indexes = uber_h3_impl_::make_zeros_vector<int64_t>(uncompact_size);
  if (unlikely(uncompact(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), h3_set_size,
                         reinterpret_cast<H3Index*>(std::addressof(uncompacted_h3_indexes[0])), uncompact_size, checked_resolution))) {
    return false;
  }
  return std::move(uncompacted_h3_indexes);
}

int64_t f$UberH3$$maxUncompactSize(const array<int64_t>& h3_indexes, int64_t resolution) noexcept {
  const int32_t checked_resolution = uber_h3_impl_::check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  const array<int64_t> h3_set = uber_h3_impl_::indexes2vector(h3_indexes);
  return maxUncompactSize(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), static_cast<int32_t>(h3_set.count()), checked_resolution);
}

int64_t f$UberH3$$maxPolyfillSize(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                  int64_t resolution) noexcept {
  const int32_t checked_resolution = uber_h3_impl_::check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  uber_h3_impl_::GeoPolygonOwner polygon_owner{polygon_boundary, holes};
  return maxPolyfillSize(std::addressof(polygon_owner.getPolygon()), checked_resolution);
}
