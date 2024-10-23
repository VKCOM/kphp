// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/runtime-core/runtime-core.h"

int64_t f$UberH3$$geoToH3(double latitude, double longitude, int64_t resolution) noexcept;
std::tuple<double, double> f$UberH3$$h3ToGeo(int64_t h3_index) noexcept;
array<std::tuple<double, double>> f$UberH3$$h3ToGeoBoundary(int64_t h3_index) noexcept;

int64_t f$UberH3$$h3GetResolution(int64_t h3_index) noexcept;
int64_t f$UberH3$$h3GetBaseCell(int64_t h3_index) noexcept;
int64_t f$UberH3$$stringToH3(const string &h3_index_str) noexcept;
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
Optional<array<int64_t>> f$UberH3$$hexRanges(const array<int64_t> &h3_indexes, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$hexRing(int64_t h3_index_origin, int64_t k) noexcept;
Optional<array<int64_t>> f$UberH3$$h3Line(int64_t h3_index_start, int64_t h3_index_end) noexcept;
int64_t f$UberH3$$h3LineSize(int64_t h3_index_start, int64_t h3_index_end) noexcept;
int64_t f$UberH3$$h3Distance(int64_t h3_index_start, int64_t h3_index_end) noexcept;

int64_t f$UberH3$$h3ToParent(int64_t h3_index, int64_t parent_resolution) noexcept;
Optional<array<int64_t>> f$UberH3$$h3ToChildren(int64_t h3_index, int64_t children_resolution) noexcept;
int64_t f$UberH3$$maxH3ToChildrenSize(int64_t h3_index, int64_t children_resolution) noexcept;
int64_t f$UberH3$$h3ToCenterChild(int64_t h3_index, int64_t children_resolution) noexcept;
Optional<array<int64_t>> f$UberH3$$compact(const array<int64_t> &h3_indexes) noexcept;
Optional<array<int64_t>> f$UberH3$$uncompact(const array<int64_t> &h3_indexes, int64_t resolution) noexcept;
int64_t f$UberH3$$maxUncompactSize(const array<int64_t> &h3_indexes, int64_t resolution) noexcept;

int64_t f$UberH3$$maxPolyfillSize(const array<std::tuple<double, double>> &polygon_boundary,
                                  const array<array<std::tuple<double, double>>> &holes,
                                  int64_t resolution) noexcept;
Optional<array<int64_t>> f$UberH3$$polyfill(const array<std::tuple<double, double>> &polygon_boundary,
                                            const array<array<std::tuple<double, double>>> &holes,
                                            int64_t resolution) noexcept;