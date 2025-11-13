// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <tuple>

#include "h3/h3api.h"

#include "runtime-common/core/runtime-core.h"

namespace uber_h3_impl_ {

inline std::tuple<double, double> coord2deg(GeoCoord geo_coord) noexcept {
  return std::make_tuple(radsToDegs(geo_coord.lat), radsToDegs(geo_coord.lon));
}

inline GeoCoord deg2coord(std::tuple<double, double> geo_deg) noexcept {
  return GeoCoord{.lat = degsToRads(std::get<0>(geo_deg)), .lon = degsToRads(std::get<1>(geo_deg))};
}

inline int32_t check_k_param(int64_t k) noexcept {
  if (unlikely(k < 0 || k > int64_t{std::numeric_limits<int>::max()})) {
    php_warning("k parameter is expected to be a positive int32, got %" PRId64, k);
    return 0;
  }
  return static_cast<int32_t>(k);
}

inline int32_t check_resolution_param(int64_t resolution) noexcept {
  constexpr int64_t uber_h3_resolution_max = 15;
  if (unlikely(resolution < 0 || resolution > uber_h3_resolution_max)) {
    php_warning("resolution parameter is expected to be between [0, %" PRId64 "], got %" PRId64, uber_h3_resolution_max, resolution);
    return 0;
  }
  return static_cast<int32_t>(resolution);
}

template<class T>
array<T> make_zeros_vector(int64_t elements) noexcept {
  array<T> elements_vector{array_size{elements, true}};
  if (elements) {
    elements_vector.fill_vector(elements, 0);
  }
  return elements_vector;
}

inline array<int64_t> indexes2vector(const array<int64_t>& h3_indexes, bool always_deep_copy = false) noexcept {
  array<int64_t> h3_vector;
  if (h3_indexes.is_vector() && !always_deep_copy) {
    h3_vector = h3_indexes;
  } else {
    h3_vector.reserve(h3_indexes.count(), true);
    for (const auto& h3_index : h3_indexes) {
      h3_vector.emplace_back(h3_index.get_value());
    }
  }
  return h3_vector;
}

class GeoPolygonOwner {
public:
  GeoPolygonOwner(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes) noexcept
      : polygon_boundary_(array_size{polygon_boundary.count(), true}),
        holes_(array_size{holes.count(), true}) {
    for (const auto& boundary_vertex : polygon_boundary) {
      polygon_boundary_.emplace_back(deg2coord(boundary_vertex.get_value()));
    }
    polygon.geofence.verts = std::addressof(polygon_boundary_[0]);
    polygon.geofence.numVerts = static_cast<int32_t>(polygon_boundary_.count());

    for (const auto& hole_vertexes : holes) {
      const auto& vertexes = hole_vertexes.get_value();
      holes_.emplace_back(Geofence{.numVerts = static_cast<int32_t>(vertexes.count()), .verts = nullptr});
      holes_vertexes_.reserve(holes_vertexes_.count() + vertexes.count(), true);
      for (const auto& hole_vertex : vertexes) {
        holes_vertexes_.emplace_back(deg2coord(hole_vertex.get_value()));
      }
    }

    int32_t prev_offset = 0;
    for (auto& hole : holes_) {
      hole.get_value().verts = std::addressof(holes_vertexes_[prev_offset]);
      prev_offset = hole.get_value().numVerts;
    }
    polygon.numHoles = static_cast<int32_t>(holes_.count());
    polygon.holes = std::addressof(holes_[0]);
  }

  const GeoPolygon& getPolygon() const noexcept {
    return polygon;
  }

private:
  array<GeoCoord> polygon_boundary_;
  array<GeoCoord> holes_vertexes_;
  array<Geofence> holes_;
  GeoPolygon polygon{};
};

} // namespace uber_h3_impl_

int64_t f$UberH3$$geoToH3(double latitude, double longitude, int64_t resolution) noexcept;
std::tuple<double, double> f$UberH3$$h3ToGeo(int64_t h3_index) noexcept;
array<std::tuple<double, double>> f$UberH3$$h3ToGeoBoundary(int64_t h3_index) noexcept;

int64_t f$UberH3$$h3GetResolution(int64_t h3_index) noexcept;
int64_t f$UberH3$$h3GetBaseCell(int64_t h3_index) noexcept;
int64_t f$UberH3$$stringToH3(const string& h3_index_str) noexcept;
string f$UberH3$$h3ToString(int64_t h3_index) noexcept;
bool f$UberH3$$h3IsValid(int64_t h3_index) noexcept;
bool f$UberH3$$h3IsResClassIII(int64_t h3_index) noexcept;
bool f$UberH3$$h3IsPentagon(int64_t h3_index) noexcept;
array<int64_t> f$UberH3$$h3GetFaces(int64_t h3_index) noexcept;
int64_t f$UberH3$$maxFaceCount(int64_t h3_index) noexcept;

Optional<array<int64_t>> f$UberH3$$kRing(int64_t h3_index_origin, int64_t k) noexcept;
int64_t f$UberH3$$maxKringSize(int64_t k) noexcept;
Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$kRingDistances(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$hexRange(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$hexRangeDistances(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$hexRanges(const array<int64_t>& h3_indexes, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$hexRing(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$h3Line(int64_t h3_index_start, int64_t h3_index_end) noexcept;
int64_t f$UberH3$$h3LineSize(int64_t h3_index_start, int64_t h3_index_end) noexcept;
int64_t f$UberH3$$h3Distance(int64_t h3_index_start, int64_t h3_index_end) noexcept;

int64_t f$UberH3$$h3ToParent(int64_t h3_index, int64_t parent_resolution) noexcept;
Optional<array<int64_t>> f$UberH3$$h3ToChildren(int64_t h3_index, int64_t children_resolution) noexcept;
int64_t f$UberH3$$maxH3ToChildrenSize(int64_t h3_index, int64_t children_resolution) noexcept;
int64_t f$UberH3$$h3ToCenterChild(int64_t h3_index, int64_t children_resolution) noexcept;
Optional<array<int64_t>> f$UberH3$$compact(const array<int64_t>& h3_indexes) noexcept;
Optional<array<int64_t>> f$UberH3$$uncompact(const array<int64_t>& h3_indexes, int64_t resolution) noexcept;
int64_t f$UberH3$$maxUncompactSize(const array<int64_t>& h3_indexes, int64_t resolution) noexcept;

Optional<array<int64_t>> f$UberH3$$polyfill(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                            int64_t resolution) noexcept;
int64_t f$UberH3$$maxPolyfillSize(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                  int64_t resolution) noexcept;
