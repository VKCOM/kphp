#pragma once

#include "catboost_common.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace kphp_ml {

constexpr int KML_FILE_PREFIX = 0x718249F0;
constexpr int KML_FILE_VERSION_100 = 100;

constexpr size_t BATCH_SIZE_XGB = 8;

enum class ModelKind {
  invalid_kind,
  xgboost_trees_no_cat,
  catboost_trees,
};

enum class InputKind {
  ht_remap_str_keys_to_fvalue,
  ht_remap_int_keys_to_fvalue,
  ht_direct_int_keys_to_fvalue,
  vectors_float_and_categorial,
  vectors_float_and_categorial_multi
};

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

  bool is_leaf() const noexcept {
    return combined_value < 0;
  }
  int vec_offset_dense() const noexcept {
    return combined_value & 0xFFFF;
  }
  int vec_offset_sparse() const noexcept {
    return (combined_value & 0xFFFF) >> 1;
  }
  int left_child() const noexcept {
    return combined_value >> 16;
  }
  bool default_left() const noexcept {
    return combined_value & 1;
  }
};

struct XgbTree {
  std::vector<XgbTreeNode> nodes;
};

struct XgbModel {
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
  // todo emhash?
  std::unordered_map<uint64_t, int> reindex_map_str2int;
  // to accept input_kind = ht_remap_int_keys_to_fvalue
  // see below, same format
  // todo do we need to accept all inputs? or input_kind is a part of model => offset_in_vec can be reused?
  int *reindex_map_int2int;
  // to accept input_kind = ht_direct_int_keys_to_fvalue
  // looks like [-1, vec_offset, -1, -1, ...]
  // any feature_id can be looked up as offset_in_vec[feature_id]:
  // * -1 means "feature is not used in a model (in any tree)"
  // * otherwise, it's used to access vector_x, see XgbDensePredictor
  int *offset_in_vec;

  // for ModelKind::xgboost_ht_remap
  bool skip_zeroes;

  float transform_base_score() const noexcept;
  double transform_prediction(double score) const noexcept;
};

using CbModel = cb_common::AOSCatboostModel;

struct MLModel {
  ModelKind model_kind;
  InputKind input_kind;
  std::string model_name;

  std::variant<XgbModel, CbModel> impl;
};

} // namespace kphp_ml
