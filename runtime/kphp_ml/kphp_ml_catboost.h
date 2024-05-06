// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>


namespace kphp_ml { struct MLModel; }
struct InputRow;

namespace kphp_ml_catboost {

enum class CatboostModelCtrType {
  Borders,
  Buckets,
  BinarizedTargetMeanValue,
  FloatTargetMeanValue,
  Counter,
  FeatureFreq,
  CtrTypesCount,
};

struct CatboostModelCtr {
  uint64_t base_hash;
  CatboostModelCtrType base_ctr_type;
  int target_border_idx;
  float prior_num;
  float prior_denom;
  float shift;
  float scale;

  float calc(float count_in_class, float total_count) const noexcept {
    float ctr = (count_in_class + prior_num) / (total_count + prior_denom);
    return (ctr + shift) * scale;
  }

  float calc(int count_in_class, int total_count) const noexcept {
    return calc(static_cast<float>(count_in_class), static_cast<float>(total_count));
  }
};

struct CatboostBinFeatureIndexValue {
  int bin_index;
  bool check_value_equal;
  unsigned char value;
};

struct CatboostCtrMeanHistory {
  float sum;
  int count;
};

struct CatboostCtrValueTable {
  std::unordered_map<uint64_t, uint32_t> index_hash_viewer;
  int target_classes_count;
  int counter_denominator;
  std::vector<CatboostCtrMeanHistory> ctr_mean_history;
  std::vector<int> ctr_total;

  const unsigned int *resolve_hash_index(uint64_t hash) const noexcept {
    auto found_it = index_hash_viewer.find(hash);
    return found_it == index_hash_viewer.end() ? nullptr : &found_it->second;
  }
};

struct CatboostCtrData {
  std::unordered_map<uint64_t, CatboostCtrValueTable> learn_ctrs;
};

struct CatboostProjection {
  std::vector<int> transposed_cat_feature_indexes;
  std::vector<CatboostBinFeatureIndexValue> binarized_indexes;
};

struct CatboostCompressedModelCtr {
  CatboostProjection projection;
  std::vector<CatboostModelCtr> model_ctrs;
};

struct CatboostModelCtrsContainer {
  int used_model_ctrs_count{0};
  std::vector<CatboostCompressedModelCtr> compressed_model_ctrs;
  CatboostCtrData ctr_data;
};

struct CatboostModel {
  struct Split {
    uint16_t feature_index;
    uint8_t xor_mask;
    uint8_t border;
  };
  static_assert(sizeof(Split) == 4);

  int float_feature_count;
  int cat_feature_count;
  int binary_feature_count;
  int tree_count;
  std::vector<int> float_features_index;
  std::vector<std::vector<float>> float_feature_borders;
  std::vector<int> tree_depth;
  std::vector<int> one_hot_cat_feature_index;
  std::vector<std::vector<int>> one_hot_hash_values;
  std::vector<std::vector<float>> ctr_feature_borders;

  std::vector<Split> tree_split;
  std::vector<float> leaf_values; // this and below are like a union
  std::vector<std::vector<float>> leaf_values_vec;

  double scale;

  double bias; // this and below are like a union
  std::vector<double> biases;

  int dimension = -1; // absent in case of NON-multiclass classification
  std::unordered_map<uint64_t, int> cat_features_hashes;

  CatboostModelCtrsContainer model_ctrs;
  // todo there are also embedded and text features, we may want to add them later

  // to accept input_kind = ht_remap_str_keys_to_fvalue_or_catstr and similar
  // 1) we store [hash => vec_idx] instead of [feature_name => vec_idx], we don't expect collisions
  // 2) this reindex_map contains both reindexes of float and categorial features, but categorial are large:
  //    [ 'emb_7' => 7, ..., 'user_age_group' => 1000001, 'user_os' => 1000002 ]
  //    the purpose of storing two maps in one is to use a single hashtable lookup when remapping
  // todo this ht is filled once (on .kml loading) and used many-many times for lookup; maybe, find smth faster than std
  std::unordered_map<uint64_t, int> reindex_map_floats_and_cat;
  static constexpr int REINDEX_MAP_CATEGORIAL_SHIFT = 1000000;
};

double kml_predict_catboost_by_vectors(const kphp_ml::MLModel &kml, const std::vector<float> &float_features, const std::vector<std::string> &cat_features, char *mutable_buffer);
double kml_predict_catboost_by_ht_remap_str_keys(const kphp_ml::MLModel &kml, const std::unordered_map<std::string, double> &features_map, char *mutable_buffer);

std::vector<double> kml_predict_catboost_by_vectors_multi(const kphp_ml::MLModel &kml, const std::vector<float> &float_features, const std::vector<std::string> &cat_features, char *mutable_buffer);
std::vector<double> kml_predict_catboost_by_ht_remap_str_keys_multi(const kphp_ml::MLModel &kml, const std::unordered_map<std::string, double> &features_map, char *mutable_buffer);


} // namespace kphp_ml_catboost
