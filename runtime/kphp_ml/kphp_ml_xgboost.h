// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include <unordered_map>
#include <vector>

#include "runtime/kphp_core.h"

/*
 * For detailed comments about KML, see kphp_ml.h.
 *
 * This module contains a custom xgboost predictor implementation.
 * It's much faster than native xgboost due to compressed layout and avoiding ifs in code.
 */

namespace kphp_ml { struct MLModel; }

namespace kphp_ml_xgboost {

constexpr int BATCH_SIZE_XGB = 8;

enum class XGTrainParamObjective {
  binary_logistic,
  rank_pairwise,
};

struct CalibrationMethod {
  enum {
    no_calibration,
    platt_scaling,
  } calibration_method{no_calibration};

  // for calibration_method = platt_scaling
  double platt_slope{1.0};
  double platt_intercept{0.0};
};

struct XgbTreeNode {
  // first 16 bits: vec_offset = vec_idx * 2 + (1 if default left)
  // last 16 bits:  left child node index (right = left + 1, always)
  int combined_value;
  float split_cond;

  bool is_leaf() const noexcept { return combined_value < 0; }
  int vec_offset_dense() const noexcept { return combined_value & 0xFFFF; }
  int vec_offset_sparse() const noexcept { return (combined_value & 0xFFFF) >> 1; }
  int left_child() const noexcept { return combined_value >> 16; }
  bool default_left() const noexcept { return combined_value & 1; }
};

struct XgbTree {
  std::vector<XgbTreeNode> nodes;
};

struct XgboostModel {
  XGTrainParamObjective tparam_objective;
  CalibrationMethod calibration;
  float base_score;
  int num_features_trained{0};
  int num_features_present{0};
  int max_required_features{0};

  std::vector<XgbTree> trees;

  // to accept input_kind = ht_remap_str_keys_to_fvalue
  // note, that the main optimization is in storing
  // [hash => vec_offset] instead of [string => vec_offset], we don't expect collisions
  // todo this ht is filled once (on .kml loading) and used many-many times for lookup; maybe, find smth faster than std
  std::unordered_map<uint64_t, int> reindex_map_str2int;
  // to accept input_kind = ht_remap_int_keys_to_fvalue
  // see below, same format
  int *reindex_map_int2int;
  // to accept input_kind = ht_direct_int_keys_to_fvalue
  // looks like [-1, vec_offset, -1, -1, ...]
  // any feature_id can be looked up as offset_in_vec[feature_id]:
  // * -1 means "feature is not used in a model (in any tree)"
  // * otherwise, it's used to access vector_x, see XgbDensePredictor
  int *offset_in_vec;

  // for ModelKind::xgboost_ht_remap
  bool skip_zeroes;
  float default_missing_value;

  float transform_base_score() const noexcept;
  double transform_prediction(double score) const noexcept;
};

array<double> kml_predict_xgboost(const kphp_ml::MLModel &kml, const array<array<double>> &in, char *mutable_buffer);

} // namespace kphp_ml_xgboost
