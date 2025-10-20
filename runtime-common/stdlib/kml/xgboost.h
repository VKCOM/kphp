// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/input_kind.h"

/*
 * For detailed comments about KML, see kphp_ml.h.
 *
 * This module contains a custom xgboost predictor implementation.
 * It's much faster than native xgboost due to compressed layout and avoiding ifs in code.
 */

namespace kphp::kml::xgboost {

inline constexpr int32_t BATCH_SIZE_XGB = 8;

enum class train_param_objective {
  binary_logistic,
  rank_pairwise,
};

struct calibration_method { // NOLINT
  enum {
    no_calibration,
    platt_scaling, // https://en.wikipedia.org/wiki/Platt_scaling
    update_prior,  // https://www.mpia.de/3432751/probcomb_TN.pdf, formulae 12, 18
  } m_calibration_method{no_calibration};

  union {
    struct {
      // for calibration_method = platt_scaling
      double m_platt_slope;
      double m_platt_intercept;
    };
    struct {
      // for calibration_method = update_prior
      double m_old_prior;
      double m_new_prior;
    };
  };
};

struct tree_node {
  // first 16 bits: vec_offset = vec_idx * 2 + (1 if default left)
  // last 16 bits:  left child node index (right = left + 1, always)
  int32_t m_combined_value;
  float m_split_cond;

  bool is_leaf() const noexcept {
    return m_combined_value < 0;
  }
  int32_t vec_offset_dense() const noexcept {
    return m_combined_value & 0xFFFF;
  }
  int32_t vec_offset_sparse() const noexcept {
    return (m_combined_value & 0xFFFF) >> 1;
  }
  int32_t left_child() const noexcept {
    return m_combined_value >> 16;
  }
  bool default_left() const noexcept {
    return m_combined_value & 1;
  }
};

static_assert(sizeof(tree_node) == 8, "unexpected sizeof(xgb_tree_node)");

template<template<class> class Allocator>
struct tree {
  kphp::stl::vector<tree_node, Allocator> m_nodes;
};

template<template<class> class Allocator>
struct model {
  train_param_objective m_tparam_objective{};
  calibration_method m_calibration;
  float m_base_score{};
  int32_t m_num_features_trained{0};
  int32_t m_num_features_present{0};
  int32_t m_max_required_features{0};

  kphp::stl::vector<tree<Allocator>, Allocator> m_trees;

  // to accept input_kind = ht_remap_str_keys_to_fvalue
  // note, that the main optimization is in storing
  // [hash => vec_offset] instead of [string => vec_offset], we don't expect collisions
  // todo this ht is filled once (on .kml loading) and used many-many times for lookup; maybe, find smth faster than std
  kphp::stl::unordered_map<uint64_t, int32_t, Allocator> m_reindex_map_str2int;
  // to accept input_kind = ht_remap_int_keys_to_fvalue
  // see below, same format
  int32_t* m_reindex_map_int2int{};
  // to accept input_kind = ht_direct_int_keys_to_fvalue
  // looks like [-1, vec_offset, -1, -1, ...]
  // any feature_id can be looked up as offset_in_vec[feature_id]:
  // * -1 means "feature is not used in a model (in any tree)"
  // * otherwise, it's used to access vector_x, see XgbDensePredictor
  int32_t* m_offset_in_vec{};

  // for ModelKind::xgboost_ht_remap
  bool m_skip_zeroes{};
  float m_default_missing_value{};

  float transform_base_score() const noexcept {
    switch (m_tparam_objective) {
    case train_param_objective::binary_logistic:
      return 1.0 / (1.0 + std::exp(-m_base_score));
    case train_param_objective::rank_pairwise:
      return m_base_score;
    default:
      __builtin_unreachable();
    }
  }

  double transform_prediction(double score) const noexcept {
    switch (m_tparam_objective) {
    case train_param_objective::binary_logistic:
      score = 1.0 / (1.0 + std::exp(-score));
    case train_param_objective::rank_pairwise:
      break;
    default:
      __builtin_unreachable();
    }

    switch (m_calibration.m_calibration_method) {
    case calibration_method::platt_scaling:
      if (m_tparam_objective == train_param_objective::binary_logistic) {
        score = -std::log(1. / score - 1);
      }
      return 1 / (1. + exp(-(m_calibration.m_platt_slope * score + m_calibration.m_platt_intercept)));
    case calibration_method::update_prior:
      return score * m_calibration.m_new_prior * (1 - m_calibration.m_old_prior) /
             (m_calibration.m_old_prior * (1 - score - m_calibration.m_new_prior) + score * m_calibration.m_new_prior);
    default:
      return score;
    }
  }

  size_t mutable_buffer_size() const noexcept {
    return static_cast<size_t>(BATCH_SIZE_XGB * m_num_features_present * 2) * sizeof(float);
  }

  template<class KMLReader>
  static std::optional<model<Allocator>> load(KMLReader& kml_reader) noexcept {
    model<Allocator> xgb{};

    kml_reader.read_enum(xgb.m_tparam_objective);
    kml_reader.read_bytes(&xgb.m_calibration, sizeof(calibration_method));
    kml_reader.read_float(xgb.m_base_score);
    kml_reader.read_int32(xgb.m_num_features_trained);
    kml_reader.read_int32(xgb.m_num_features_present);
    kml_reader.read_int32(xgb.m_max_required_features);

    if (kml_reader.is_eof()) {
      php_warning("failed to load XGBoost model: unexpected EOF");
      return std::nullopt;
    }

    if (xgb.m_num_features_present <= 0 || xgb.m_num_features_present > xgb.m_max_required_features) {
      php_warning("failed to load XGBoost model: wrong num_features_present");
      return std::nullopt;
    }

    auto num_trees = kml_reader.read_int32();
    if (num_trees <= 0 || num_trees > 10000) {
      php_warning("failed to load XGBoost model: wrong num_trees");
      return std::nullopt;
    }

    xgb.m_trees.resize(num_trees);
    for (auto& tree : xgb.m_trees) {
      auto num_nodes = kml_reader.read_int32();
      if (num_nodes <= 0 || num_nodes > 10000) {
        php_warning("failed to load XGBoost model: wrong num_nodes");
        return std::nullopt;
      }
      tree.m_nodes.resize(num_nodes);
      kml_reader.read_bytes(tree.m_nodes.data(), sizeof(tree_node) * num_nodes);
    }

    if (kml_reader.is_eof()) {
      php_warning("failed to load XGBoost model: unexpected EOF");
      return std::nullopt;
    }

    // FIXME: unmanaged memory
    xgb.m_offset_in_vec = Allocator<int32_t>{}.allocate(xgb.m_max_required_features);
    kml_reader.read_bytes(xgb.m_offset_in_vec, xgb.m_max_required_features * sizeof(int32_t));

    if (kml_reader.is_eof()) {
      php_warning("failed to load XGBoost model: unexpected EOF");
      return std::nullopt;
    }

    // FIXME: unmanaged memory
    xgb.m_reindex_map_int2int = Allocator<int32_t>{}.allocate(xgb.m_max_required_features);
    kml_reader.read_bytes(xgb.m_reindex_map_int2int, xgb.m_max_required_features * sizeof(int32_t));

    if (kml_reader.is_eof()) {
      php_warning("failed to load XGBoost model: unexpected EOF");
      return std::nullopt;
    }

    auto num_reindex_str2int = kml_reader.read_int32();
    if (num_reindex_str2int < 0 || num_reindex_str2int > xgb.m_max_required_features) {
      php_warning("failed to load XGBoost model: wrong num_reindex_str2int");
      return std::nullopt;
    }
    for (auto i = 0; i < num_reindex_str2int; ++i) {
      uint64_t hash = 0;
      kml_reader.read_uint64(hash);
      kml_reader.read_int32(xgb.m_reindex_map_str2int[hash]);
    }

    if (kml_reader.is_eof()) {
      php_warning("failed to load XGBoost model: unexpected EOF");
      return std::nullopt;
    }

    kml_reader.read_bool(xgb.m_skip_zeroes);
    kml_reader.read_float(xgb.m_default_missing_value);

    return xgb;
  }
};

namespace detail {

template<template<class> class Allocator>
struct dense_predictor {
  [[gnu::always_inline]] static inline uint64_t default_missing_value(const kphp::kml::xgboost::model<Allocator>& model) noexcept {
    std::pair<float, float> default_missing_value_layout;
    if (std::isnan(model.m_default_missing_value)) {
      // Should be +/- infinity, but it would be much slower, just +/- "big" value
      default_missing_value_layout = {+1e10, -1e10};
    } else {
      default_missing_value_layout = {model.m_default_missing_value, model.m_default_missing_value};
    }

    return *reinterpret_cast<uint64_t*>(std::addressof(default_missing_value_layout));
  }

  float* vector_x{nullptr}; // assigned outside as a chunk in linear memory, 2 equal values per existing feature

  using filler_int_keys = void (dense_predictor::*)(const kphp::kml::xgboost::model<Allocator>&, const array<double>&) const noexcept;
  using filler_str_keys = void (dense_predictor::*)(const kphp::kml::xgboost::model<Allocator>&, const array<double>&) const noexcept;

  void fill_vector_x_ht_direct(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(kv.is_string_key(), false)) {
        continue;
      }
      const int32_t feature_id = kv.get_int_key();
      const double fvalue = kv.get_value();

      if (feature_id < 0 || feature_id >= xgb.m_max_required_features) { // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }

      int32_t vec_offset = xgb.m_offset_in_vec[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_direct_sz(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(kv.is_string_key(), false)) {
        continue;
      }
      const int32_t feature_id = kv.get_int_key();
      const double fvalue = kv.get_value();

      if (feature_id < 0 || feature_id >= xgb.m_max_required_features) { // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }
      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int32_t vec_offset = xgb.m_offset_in_vec[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_str_key(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(!kv.is_string_key(), false)) {
        continue;
      }
      const string& feature_name = kv.get_string_key();
      const double fvalue = kv.get_value();

      auto found_it = xgb.m_reindex_map_str2int.find(string_hash(feature_name.c_str(), feature_name.size()));
      if (found_it != xgb.m_reindex_map_str2int.end()) { // input contains [ "unexisting_feature" => 0.123 ], it's ok
        int32_t vec_offset = found_it->second;
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_str_key_sz(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(!kv.is_string_key(), false)) {
        continue;
      }
      const string& feature_name = kv.get_string_key();
      const double fvalue = kv.get_value();

      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      auto found_it = xgb.m_reindex_map_str2int.find(string_hash(feature_name.c_str(), feature_name.size()));
      if (found_it != xgb.m_reindex_map_str2int.end()) { // input contains [ "unexisting_feature" => 0.123 ], it's ok
        int32_t vec_offset = found_it->second;
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_int_key(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(kv.is_string_key(), false)) {
        continue;
      }
      const int32_t feature_id = kv.get_int_key();
      const double fvalue = kv.get_value();

      if (feature_id < 0 || feature_id >= xgb.m_max_required_features) { // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }

      int32_t vec_offset = xgb.m_reindex_map_int2int[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_int_key_sz(const kphp::kml::xgboost::model<Allocator>& xgb, const array<double>& features_map) const noexcept {
    for (const auto& kv : features_map) {
      if (__builtin_expect(kv.is_string_key(), false)) {
        continue;
      }
      const int32_t feature_id = kv.get_int_key();
      const double fvalue = kv.get_value();

      if (feature_id < 0 || feature_id >= xgb.m_max_required_features) { // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }
      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int32_t vec_offset = xgb.m_reindex_map_int2int[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  float predict_one_tree(const kphp::kml::xgboost::tree<Allocator>& tree) const noexcept {
    const auto* node = tree.m_nodes.data();
    while (!node->is_leaf()) {
      bool goto_right = vector_x[node->vec_offset_dense()] >= node->m_split_cond;
      node = &tree.m_nodes[node->left_child() + goto_right];
    }
    return node->m_split_cond;
  }
};

} // namespace detail

template<template<class> class Allocator>
array<double> predict(const kphp::kml::xgboost::model<Allocator>& xgb, kphp::kml::input_kind input_kind, const array<array<double>>& in,
                      std::byte* mutable_buf) noexcept {
  int32_t n_rows = static_cast<int32_t>(in.size().size);

  typename detail::dense_predictor<Allocator>::filler_int_keys filler_int_keys{nullptr};
  typename detail::dense_predictor<Allocator>::filler_str_keys filler_str_keys{nullptr};
  switch (input_kind) {
  case input_kind::ht_direct_int_keys_to_fvalue:
    filler_int_keys =
        xgb.m_skip_zeroes ? &detail::dense_predictor<Allocator>::fill_vector_x_ht_direct_sz : &detail::dense_predictor<Allocator>::fill_vector_x_ht_direct;
    break;
  case input_kind::ht_remap_int_keys_to_fvalue:
    filler_int_keys = xgb.m_skip_zeroes ? &detail::dense_predictor<Allocator>::fill_vector_x_ht_remap_int_key_sz
                                        : &detail::dense_predictor<Allocator>::fill_vector_x_ht_remap_int_key;
    break;
  case input_kind::ht_remap_str_keys_to_fvalue:
    filler_str_keys = xgb.m_skip_zeroes ? &detail::dense_predictor<Allocator>::fill_vector_x_ht_remap_str_key_sz
                                        : &detail::dense_predictor<Allocator>::fill_vector_x_ht_remap_str_key;
    break;
  default:
    php_assert(0 && "invalid input_kind"); // FIXME avoid using php_assert or assert
    return {};
  }

  detail::dense_predictor<Allocator> feat_vecs[BATCH_SIZE_XGB];
  for (int32_t i = 0; i < BATCH_SIZE_XGB && i < n_rows; ++i) {
    feat_vecs[i].vector_x = reinterpret_cast<float*>(mutable_buf) + i * xgb.m_num_features_present * 2;
  }
  auto iter_done = in.begin();

  const float base_score = xgb.transform_base_score();
  array<double> out_predictions;
  out_predictions.reserve(n_rows, true);
  for (int32_t i = 0; i < n_rows; ++i) {
    out_predictions.push_back(base_score);
  }

  const auto n_batches = static_cast<int32_t>(std::ceil(static_cast<double>(n_rows) / BATCH_SIZE_XGB));

  for (int32_t block_id = 0; block_id < n_batches; ++block_id) {
    const int32_t batch_offset = block_id * BATCH_SIZE_XGB;
    const int32_t block_size = std::min(n_rows - batch_offset, BATCH_SIZE_XGB);

    std::fill_n(reinterpret_cast<uint64_t*>(mutable_buf), block_size * xgb.m_num_features_present,
                detail::dense_predictor<Allocator>::default_missing_value(xgb));
    if (filler_int_keys != nullptr) {
      for (int32_t i = 0; i < block_size; ++i) {
        (feat_vecs[i].*filler_int_keys)(xgb, iter_done.get_value());
        ++iter_done;
      }
    } else {
      for (int32_t i = 0; i < block_size; ++i) {
        (feat_vecs[i].*filler_str_keys)(xgb, iter_done.get_value());
        ++iter_done;
      }
    }

    for (const auto& tree : xgb.m_trees) {
      for (int32_t i = 0; i < block_size; ++i) {
        out_predictions[batch_offset + i] += feat_vecs[i].predict_one_tree(tree);
      }
    }
  }

  for (int32_t i = 0; i < n_rows; ++i) {
    out_predictions[i] = xgb.transform_prediction(out_predictions[i]);
  }

  return out_predictions;
}

} // namespace kphp::kml::xgboost
