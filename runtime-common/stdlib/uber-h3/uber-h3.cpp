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

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"

/*
 * This is necessary to replace the library's allocators.
 * When building the library, the prefix of the replacement function is specified, for example:
 * if we want to use the prefix of the replacement function = "my_prefix_", then during assembly we specify
 *    cmake -DH3_ALLOC_PREFIX=my_prefix_ .
 * then it will use
 *   void* my_prefix_malloc(size_t size);
 *   void* my_prefix_calloc(size_t num, size_t size);
 *   void* my_prefix_realloc(void* ptr, size_t size);
 *   void my_prefix_free(void* ptr);
 * instead of
 *   void* malloc(size_t size);
 *   void* calloc(size_t num, size_t size);
 *   void* realloc(void* ptr, size_t size);
 *   void free(void* ptr);
 */
extern "C" {
void* uber_h3_malloc(size_t size) {
  return kphp::memory::script::alloc(size);
}
void* uber_h3_calloc(size_t num, size_t size) {
  return kphp::memory::script::calloc(num, size);
}
void* uber_h3_realloc(void* ptr, size_t size) {
  return kphp::memory::script::realloc(ptr, size);
}
void uber_h3_free(void* ptr) {
  kphp::memory::script::free(ptr);
}
}

namespace {

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

} // namespace

int64_t f$UberH3$$geoToH3(double latitude, double longitude, int64_t resolution) noexcept {
  static_assert(std::is_same<H3Index, uint64_t>{}, "H3Index expected to be uint64_t");
  const int32_t checked_resolution = check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  const auto geo_cord = deg2coord(std::make_tuple(latitude, longitude));
  return geoToH3(std::addressof(geo_cord), checked_resolution);
}

std::tuple<double, double> f$UberH3$$h3ToGeo(int64_t h3_index) noexcept {
  GeoCoord geo_coord{};
  h3ToGeo(h3_index, std::addressof(geo_coord));
  return coord2deg(geo_coord);
}

array<std::tuple<double, double>> f$UberH3$$h3ToGeoBoundary(int64_t h3_index) noexcept {
  GeoBoundary boundary;
  h3ToGeoBoundary(h3_index, std::addressof(boundary));
  array<std::tuple<double, double>> result{array_size{boundary.numVerts, true}};
  for (int i = 0; i < boundary.numVerts; ++i) {
    result.emplace_back(coord2deg(boundary.verts[i]));
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
  auto int32_result = make_zeros_vector<int32_t>(face_count);
  if (face_count) {
    h3GetFaces(h3_index, std::addressof(int32_result[0]));
  }
  return array<int64_t>::convert_from(int32_result);
}

int64_t f$UberH3$$maxFaceCount(int64_t h3_index) noexcept {
  return maxFaceCount(static_cast<H3Index>(h3_index));
}

Optional<array<int64_t>> f$UberH3$$kRing(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count = maxKringSize(checked_k);
  auto neighbor_indexes = make_zeros_vector<int64_t>(neighbors_count);
  if (neighbors_count) {
    kRing(h3_index_origin, checked_k, reinterpret_cast<H3Index*>(std::addressof(neighbor_indexes[0])));
  }
  return std::move(neighbor_indexes);
}

int64_t f$UberH3$$maxKringSize(int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  return checked_k != k ? 0 : maxKringSize(checked_k);
}

Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$kRingDistances(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count = maxKringSize(checked_k);
  array<std::tuple<int64_t, int64_t>> result{array_size{neighbors_count, true}};
  if (neighbors_count) {
    auto neighbor_indexes = make_zeros_vector<H3Index>(neighbors_count);
    auto neighbor_distances = make_zeros_vector<int32_t>(neighbors_count);
    kRingDistances(h3_index_origin, checked_k, std::addressof(neighbor_indexes[0]), std::addressof(neighbor_distances[0]));
    for (int i = 0; i < neighbors_count; ++i) {
      result.emplace_back(std::make_tuple(neighbor_indexes[i], neighbor_distances[i]));
    }
  }
  return std::move(result);
}

Optional<array<int64_t>> f$UberH3$$hexRange(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto neighbors = make_zeros_vector<int64_t>(maxKringSize(checked_k));
  if (!neighbors.empty()) {
    if (unlikely(hexRange(h3_index_origin, checked_k, reinterpret_cast<H3Index*>(std::addressof(neighbors[0]))))) {
      return false;
    }
  }
  return std::move(neighbors);
}

Optional<array<std::tuple<int64_t, int64_t>>> f$UberH3$$hexRangeDistances(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  const int32_t neighbors_count = maxKringSize(checked_k);
  array<std::tuple<int64_t, int64_t>> result{array_size{neighbors_count, true}};
  if (neighbors_count) {
    auto neighbor_indexes = make_zeros_vector<H3Index>(neighbors_count);
    auto neighbor_distances = make_zeros_vector<int32_t>(neighbors_count);
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
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto h3_indexes_set = indexes2vector(h3_indexes, true);
  auto h3_indexes_result = make_zeros_vector<int64_t>(maxKringSize(checked_k) * h3_indexes.count());
  if (!h3_indexes_result.empty()) {
    if (unlikely(hexRanges(reinterpret_cast<H3Index*>(std::addressof(h3_indexes_set[0])), static_cast<int32_t>(h3_indexes.count()), checked_k,
                           reinterpret_cast<H3Index*>(std::addressof(h3_indexes_result[0]))))) {
      return false;
    }
  }
  return std::move(h3_indexes_result);
}

Optional<array<int64_t>> f$UberH3$$hexRing(int64_t h3_index_origin, int64_t k) noexcept {
  const int32_t checked_k = check_k_param(k);
  if (unlikely(checked_k != k)) {
    return false;
  }

  auto h3_indexes_result = make_zeros_vector<int64_t>(checked_k ? checked_k * 6 : 1);
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

  auto line = make_zeros_vector<int64_t>(size);
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
  const int32_t checked_parent_resolution = check_resolution_param(parent_resolution);
  return checked_parent_resolution != parent_resolution ? 0 : h3ToParent(static_cast<H3Index>(h3_index), checked_parent_resolution);
}

Optional<array<int64_t>> f$UberH3$$h3ToChildren(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = check_resolution_param(children_resolution);
  if (unlikely(checked_children_resolution != children_resolution)) {
    return false;
  }
  const int64_t children_count = maxH3ToChildrenSize(static_cast<H3Index>(h3_index), checked_children_resolution);
  auto children = make_zeros_vector<int64_t>(children_count);
  if (children_count) {
    h3ToChildren(static_cast<H3Index>(h3_index), checked_children_resolution, reinterpret_cast<H3Index*>(std::addressof(children[0])));
  }
  return std::move(children);
}

int64_t f$UberH3$$maxH3ToChildrenSize(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = check_resolution_param(children_resolution);
  return checked_children_resolution != children_resolution ? 0 : maxH3ToChildrenSize(static_cast<H3Index>(h3_index), checked_children_resolution);
}

int64_t f$UberH3$$h3ToCenterChild(int64_t h3_index, int64_t children_resolution) noexcept {
  const int32_t checked_children_resolution = check_resolution_param(children_resolution);
  return checked_children_resolution != children_resolution ? 0 : h3ToCenterChild(static_cast<H3Index>(h3_index), checked_children_resolution);
}

Optional<array<int64_t>> f$UberH3$$compact(const array<int64_t>& h3_indexes) noexcept {
  const array<int64_t> h3_set = indexes2vector(h3_indexes);
  auto compacted_h3_set = make_zeros_vector<int64_t>(h3_set.count());
  if (!compacted_h3_set.empty()) {
    if (unlikely(compact(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), reinterpret_cast<H3Index*>(std::addressof(compacted_h3_set[0])),
                         static_cast<int32_t>(h3_indexes.count())))) {
      return false;
    }
  }
  int64_t left = compacted_h3_set.count();
  while (left && !compacted_h3_set[--left]) {
    compacted_h3_set.pop();
  }
  return std::move(compacted_h3_set);
}

Optional<array<int64_t>> f$UberH3$$uncompact(const array<int64_t>& h3_indexes, int64_t resolution) noexcept {
  const int32_t checked_resolution = check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return false;
  }

  const auto h3_set_size = static_cast<int32_t>(h3_indexes.count());
  const array<int64_t> h3_set = indexes2vector(h3_indexes);
  const int32_t uncompact_size = maxUncompactSize(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), h3_set_size, checked_resolution);
  if (unlikely(uncompact_size < 0)) {
    return false;
  }
  if (!uncompact_size) {
    return array<int64_t>{};
  }
  auto uncompacted_h3_indexes = make_zeros_vector<int64_t>(uncompact_size);
  if (unlikely(uncompact(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), h3_set_size,
                         reinterpret_cast<H3Index*>(std::addressof(uncompacted_h3_indexes[0])), uncompact_size, checked_resolution))) {
    return false;
  }
  return std::move(uncompacted_h3_indexes);
}

int64_t f$UberH3$$maxUncompactSize(const array<int64_t>& h3_indexes, int64_t resolution) noexcept {
  const int32_t checked_resolution = check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  const array<int64_t> h3_set = indexes2vector(h3_indexes);
  return maxUncompactSize(reinterpret_cast<const H3Index*>(h3_set.get_const_vector_pointer()), static_cast<int32_t>(h3_set.count()), checked_resolution);
}

int64_t f$UberH3$$maxPolyfillSize(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                  int64_t resolution) noexcept {
  const int32_t checked_resolution = check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return 0;
  }
  GeoPolygonOwner polygon_owner{polygon_boundary, holes};
  return maxPolyfillSize(std::addressof(polygon_owner.getPolygon()), checked_resolution);
}

Optional<array<int64_t>> f$UberH3$$polyfill(const array<std::tuple<double, double>>& polygon_boundary, const array<array<std::tuple<double, double>>>& holes,
                                            int64_t resolution) noexcept {
  const int32_t checked_resolution = check_resolution_param(resolution);
  if (unlikely(checked_resolution != resolution)) {
    return false;
  }

  GeoPolygonOwner polygon_owner{polygon_boundary, holes};
  const int32_t max_size = maxPolyfillSize(std::addressof(polygon_owner.getPolygon()), checked_resolution);
  if (max_size < 0) {
    return false;
  }
  auto hexagon_indexes = make_zeros_vector<int64_t>(max_size);
  if (!hexagon_indexes.empty()) {
    polyfill(std::addressof(polygon_owner.getPolygon()), checked_resolution, reinterpret_cast<H3Index*>(std::addressof(hexagon_indexes[0])));
  }
  int64_t indexes_count = 0;
  for (const auto& element : hexagon_indexes) {
    indexes_count += element.get_value() ? 1 : 0;
  }
  array<int64_t> result_array{array_size{indexes_count, true}};
  for (const auto& element : hexagon_indexes) {
    if (auto h3_index = element.get_value()) {
      result_array.emplace_back(h3_index);
    }
  }

  return std::move(result_array);
}
