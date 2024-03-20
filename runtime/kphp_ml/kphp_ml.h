#pragma once

#include <string>
#include <variant>

#include "kphp_ml/kphp_ml_catboost.h"
#include "kphp_ml/kphp_ml_xgboost.h"

namespace kphp_ml {

constexpr int KML_FILE_PREFIX = 0x718249F0;
constexpr int KML_FILE_VERSION_100 = 100;

enum class ModelKind {
  invalid_kind,
  xgboost_trees_no_cat,
  catboost_trees,
};

enum class InputKind {
  // [ 'user_city_99' => 1, 'user_topic_weights_17' => 7.42, ...], uses reindex_map
  ht_remap_str_keys_to_fvalue,
  // [ 12934 => 1, 8923 => 7.42, ... ], uses reindex_map
  ht_remap_int_keys_to_fvalue,
  // [ 70 => 1, 23 => 7.42, ... ], no keys reindex, pass directly
  ht_direct_int_keys_to_fvalue,

  // [ 1.23, 4.56, ... ] and [ "red", "small" ]: floats and cat separately, pass directly
  vectors_fvalue_and_catstr,
  // the same, but a model is a catboost multiclassificator (returns an array of predictions, not one)
  vectors_fvalue_and_catstr_multi,
  // [ 'emb_7' => 19.98, ..., 'user_os' => 2, ... ]: in one ht, but categorials are numbers also (to avoid `mixed`)
  ht_remap_str_keys_to_fvalue_or_catnum,
  // same as catnum, but multiclassificator (returns an array of predictions, not one)
  ht_remap_str_keys_to_fvalue_or_catnum_multi,
};

struct MLModel {
  ModelKind model_kind;
  InputKind input_kind;
  std::string model_name;

  std::variant<kphp_ml_xgboost::XgboostModel, kphp_ml_catboost::CatboostModel> impl;

  bool is_xgboost() const { return model_kind == ModelKind::xgboost_trees_no_cat; }
  bool is_catboost() const { return model_kind == ModelKind::catboost_trees; }
  bool is_catboost_multi_classification() const;

  unsigned int calculate_mutable_buffer_size() const;
};

} // namespace kphp_ml
