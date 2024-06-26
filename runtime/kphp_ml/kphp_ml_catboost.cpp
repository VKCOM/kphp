// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#include "runtime/kphp_ml/kphp_ml_catboost.h"

#include "runtime-core/runtime-core.h"
#include "runtime/kphp_ml/kphp_ml.h"

/*
 * For detailed comments about KML, see kphp_ml.h.
 *
 * This module contains a custom catboost predictor implementation.
 *
 * Note, that catboost models support not only float features, but categorial features also (ctrs).
 * There are many structs related to ctrs, which are almost copied as-is from catboost sources,
 * without any changes or renamings for better mapping onto original sources.
 * https://github.com/catboost/catboost/blob/master/catboost/libs/model/model_export/resources/
 *
 * Text features and embedded features are not supported by kml yet, we had no need of.
 */

namespace kphp_ml_catboost {

static inline uint64_t calc_hash(uint64_t a, uint64_t b) {
  static constexpr uint64_t MAGIC_MULT = 0x4906ba494954cb65ull;
  return MAGIC_MULT * (a + MAGIC_MULT * b);
}

static uint64_t calc_hashes(const unsigned char *binarized_features,
                            const int *hashed_cat_features,
                            const std::vector<int> &transposed_cat_feature_indexes,
                            const std::vector<CatboostBinFeatureIndexValue> &binarized_feature_indexes) {
  uint64_t result = 0;
  for (int cat_feature_index: transposed_cat_feature_indexes) {
    result = calc_hash(result, static_cast<uint64_t>(hashed_cat_features[cat_feature_index]));
  }
  for (const CatboostBinFeatureIndexValue &bin_feature_index: binarized_feature_indexes) {
    unsigned char binary_feature = binarized_features[bin_feature_index.bin_index];
    if (!bin_feature_index.check_value_equal) {
      result = calc_hash(result, binary_feature >= bin_feature_index.value);
    } else {
      result = calc_hash(result, binary_feature == bin_feature_index.value);
    }
  }
  return result;
}

static void calc_ctrs(const CatboostModelCtrsContainer &model_ctrs,
                      const unsigned char *binarized_features,
                      const int *hashed_cat_features,
                      float *result) {
  int result_index = 0;

  for (int i = 0; i < model_ctrs.compressed_model_ctrs.size(); ++i) {
    const CatboostProjection &proj = model_ctrs.compressed_model_ctrs[i].projection;
    uint64_t ctr_hash = calc_hashes(binarized_features, hashed_cat_features, proj.transposed_cat_feature_indexes, proj.binarized_indexes);

    for (int j = 0; j < model_ctrs.compressed_model_ctrs[i].model_ctrs.size(); ++j, ++result_index) {
      const CatboostModelCtr &ctr = model_ctrs.compressed_model_ctrs[i].model_ctrs[j];
      const CatboostCtrValueTable &learn_ctr = model_ctrs.ctr_data.learn_ctrs.at(ctr.base_hash);
      CatboostModelCtrType ctr_type = ctr.base_ctr_type;
      const auto *bucket_ptr = learn_ctr.resolve_hash_index(ctr_hash);
      if (bucket_ptr == nullptr) {
        result[result_index] = ctr.calc(0, 0);
      } else {
        unsigned int bucket = *bucket_ptr;
        if (ctr_type == CatboostModelCtrType::BinarizedTargetMeanValue || ctr_type == CatboostModelCtrType::FloatTargetMeanValue) {
          const CatboostCtrMeanHistory &ctr_mean_history = learn_ctr.ctr_mean_history[bucket];
          result[result_index] = ctr.calc(ctr_mean_history.sum, static_cast<float>(ctr_mean_history.count));
        } else if (ctr_type == CatboostModelCtrType::Counter || ctr_type == CatboostModelCtrType::FeatureFreq) {
          const std::vector<int> &ctr_total = learn_ctr.ctr_total;
          result[result_index] = ctr.calc(ctr_total[bucket], learn_ctr.counter_denominator);
        } else if (ctr_type == CatboostModelCtrType::Buckets) {
          const std::vector<int> &ctr_history = learn_ctr.ctr_total;
          int target_classes_count = learn_ctr.target_classes_count;
          int good_count = ctr_history[bucket * target_classes_count + ctr.target_border_idx];
          int total_count = 0;
          for (int classId = 0; classId < target_classes_count; ++classId) {
            total_count += ctr_history[bucket * target_classes_count + classId];
          }
          result[result_index] = ctr.calc(good_count, total_count);
        } else {
          const std::vector<int> &ctr_history = learn_ctr.ctr_total;
          int target_classes_count = learn_ctr.target_classes_count;
          if (target_classes_count > 2) {
            int good_count = 0;
            int total_count = 0;
            for (int class_id = 0; class_id < ctr.target_border_idx + 1; ++class_id) {
              total_count += ctr_history[bucket * target_classes_count + class_id];
            }
            for (int class_id = ctr.target_border_idx + 1; class_id < target_classes_count; ++class_id) {
              good_count += ctr_history[bucket * target_classes_count + class_id];
            }
            total_count += good_count;
            result[result_index] = ctr.calc(good_count, total_count);
          } else {
            result[result_index] = ctr.calc(ctr_history[bucket * 2 + 1], ctr_history[bucket * 2] + ctr_history[bucket * 2 + 1]);
          }
        }
      }
    }
  }
}

static int get_hash(const string &cat_feature, const std::unordered_map<uint64_t, int> &cat_feature_hashes) {
  auto found_it = cat_feature_hashes.find(string_hash(cat_feature.c_str(), cat_feature.size()));
  return found_it == cat_feature_hashes.end() ? 0x7fffffff : found_it->second;
}

template<class FloatOrDouble>
static double predict_one(const CatboostModel &cbm,
                          const FloatOrDouble *float_features,
                          const string *cat_features,
                          char *mutable_buffer) {
  char *p_buffer = mutable_buffer;

  // Binarise features
  auto *binary_features = reinterpret_cast<unsigned char *>(mutable_buffer);
  p_buffer += (cbm.binary_feature_count + 4 - 1) / 4 * 4; // round up to 4 bytes

  int binary_feature_index = -1;

  // binarize float features
  for (int i = 0; i < cbm.float_feature_borders.size(); ++i) {
    int cur = 0;
    for (double border: cbm.float_feature_borders[i]) {
      cur += float_features[cbm.float_features_index[i]] > border;
    }
    binary_features[++binary_feature_index] = cur;
  }

  auto *transposed_hash = reinterpret_cast<int *>(p_buffer);
  p_buffer += cbm.cat_feature_count * sizeof(int);
  for (int i = 0; i < cbm.cat_feature_count; ++i) {
    transposed_hash[i] = get_hash(cat_features[i], cbm.cat_features_hashes);
  }

  // binarize one hot cat features
  // note, that it slightly differs from original: one_hot_cat_feature_index is precomputed in converting to kml, no repack needed
  for (int i = 0; i < cbm.one_hot_cat_feature_index.size(); ++i) {
    int cat_idx = cbm.one_hot_cat_feature_index[i];
    int hash = transposed_hash[cat_idx];
    int cur = 0;
    for (int border_idx = 0; border_idx < cbm.one_hot_hash_values[i].size(); ++border_idx) {
      cur |= (hash == cbm.one_hot_hash_values[i][border_idx]) * (border_idx + 1);
    }
    binary_features[++binary_feature_index] = cur;
  }

  // binarize ctr features
  if (cbm.model_ctrs.used_model_ctrs_count > 0) {
    auto *ctrs = reinterpret_cast<float *>(p_buffer);
    p_buffer += cbm.model_ctrs.used_model_ctrs_count * sizeof(float);
    calc_ctrs(cbm.model_ctrs, binary_features, transposed_hash, ctrs);

    for (int i = 0; i < cbm.ctr_feature_borders.size(); ++i) {
      unsigned char cur = 0;
      for (float border: cbm.ctr_feature_borders[i]) {
        cur += ctrs[i] > border;
      }
      binary_features[++binary_feature_index] = cur;
    }
  }

  // Extract and sum values from trees

  double result = 0.;
  int tree_splits_index = 0;
  int current_tree_leaf_values_index = 0;

  for (int tree_id = 0; tree_id < cbm.tree_count; ++tree_id) {
    int current_tree_depth = cbm.tree_depth[tree_id];
    int index = 0;
    for (int depth = 0; depth < current_tree_depth; ++depth) {
      const CatboostModel::Split &split = cbm.tree_split[tree_splits_index + depth];
      index |= ((binary_features[split.feature_index] ^ split.xor_mask) >= split.border) << depth;
    }
    result += cbm.leaf_values[current_tree_leaf_values_index + index];
    tree_splits_index += current_tree_depth;
    current_tree_leaf_values_index += 1 << current_tree_depth;
  }

  return cbm.scale * result + cbm.bias;
}

template<class FloatOrDouble>
static array<double> predict_one_multi(const CatboostModel &cbm,
                                       const FloatOrDouble *float_features,
                                       const string *cat_features,
                                       char *mutable_buffer) {
  char *p_buffer = mutable_buffer;

  // Binarise features
  auto *binary_features = reinterpret_cast<unsigned char *>(mutable_buffer);
  p_buffer += (cbm.binary_feature_count + 4 - 1) / 4 * 4; // round up to 4 bytes

  int binary_feature_index = -1;

  // binarize float features
  for (int i = 0; i < cbm.float_feature_borders.size(); ++i) {
    int cur = 0;
    for (double border: cbm.float_feature_borders[i]) {
      cur += float_features[cbm.float_features_index[i]] > border;
    }
    binary_features[++binary_feature_index] = cur;
  }

  auto *transposed_hash = reinterpret_cast<int *>(p_buffer);
  p_buffer += cbm.cat_feature_count * sizeof(int);
  for (int i = 0; i < cbm.cat_feature_count; ++i) {
    transposed_hash[i] = get_hash(cat_features[i], cbm.cat_features_hashes);
  }

  // binarize one hot cat features
  // note, that it slightly differs from original: one_hot_cat_feature_index is precomputed in converting to kml, no repack needed
  for (int i = 0; i < cbm.one_hot_cat_feature_index.size(); ++i) {
    int cat_idx = cbm.one_hot_cat_feature_index[i];
    int hash = transposed_hash[cat_idx];
    int cur = 0;
    for (int border_idx = 0; border_idx < cbm.one_hot_hash_values[i].size(); ++border_idx) {
      cur |= (hash == cbm.one_hot_hash_values[i][border_idx]) * (border_idx + 1);
    }
    binary_features[++binary_feature_index] = cur;
  }

  // binarize ctr features
  if (cbm.model_ctrs.used_model_ctrs_count > 0) {
    auto *ctrs = reinterpret_cast<float *>(p_buffer);
    p_buffer += cbm.model_ctrs.used_model_ctrs_count * sizeof(float);
    calc_ctrs(cbm.model_ctrs, binary_features, transposed_hash, ctrs);

    for (int i = 0; i < cbm.ctr_feature_borders.size(); ++i) {
      int cur = 0;
      for (float border: cbm.ctr_feature_borders[i]) {
        cur += ctrs[i] > border;
      }
      binary_features[++binary_feature_index] = cur;
    }
  }

  // Extract and sum values from trees

  array<double> results(array_size(cbm.dimension, true));
  for (int i = 0; i < cbm.dimension; ++i) {
    results[i] = 0.0;
  }

  const std::vector<float> *leaf_values_ptr = cbm.leaf_values_vec.data();
  int tree_ptr = 0;

  for (int tree_id = 0; tree_id < cbm.tree_count; ++tree_id) {
    int current_tree_depth = cbm.tree_depth[tree_id];
    int index = 0;
    for (int depth = 0; depth < current_tree_depth; ++depth) {
      const CatboostModel::Split &split = cbm.tree_split[tree_ptr + depth];
      index |= ((binary_features[split.feature_index] ^ split.xor_mask) >= split.border) << depth;
    }
    for (int i = 0; i < cbm.dimension; ++i) {
      results[i] += leaf_values_ptr[index][i];
    }
    tree_ptr += current_tree_depth;
    leaf_values_ptr += 1 << current_tree_depth;
  }
  for (int i = 0; i < cbm.dimension; ++i) {
    results[i] = results[i] * cbm.scale + cbm.biases[i];
  }

  return results;
}

double kml_predict_catboost_by_vectors(const kphp_ml::MLModel &kml,
                                       const array<double> &float_features,
                                       const array<string> &cat_features,
                                       char *mutable_buffer) {
  const auto &cbm = std::get<CatboostModel>(kml.impl);

  if (!float_features.is_vector() || float_features.count() < cbm.float_feature_count) {
    php_warning("incorrect input size for float_features, model %s", kml.model_name.c_str());
    return 0.0;
  }
  if (!cat_features.is_vector() || cat_features.count() < cbm.cat_feature_count) {
    php_warning("incorrect input size for cat_features, model %s", kml.model_name.c_str());
    return 0.0;
  }

  return predict_one<double>(cbm, float_features.get_const_vector_pointer(), cat_features.get_const_vector_pointer(), mutable_buffer);
}

double kml_predict_catboost_by_ht_remap_str_keys(const kphp_ml::MLModel &kml,
                                                 const array<double> &features_map,
                                                 char *mutable_buffer) {
  const auto &cbm = std::get<CatboostModel>(kml.impl);

  auto *float_features = reinterpret_cast<float *>(mutable_buffer);
  mutable_buffer += sizeof(float) * cbm.float_feature_count;
  std::fill_n(float_features, cbm.float_feature_count, 0.0);

  auto *cat_features = reinterpret_cast<string *>(mutable_buffer);
  mutable_buffer += sizeof(string) * cbm.cat_feature_count;
  for (int i = 0; i < cbm.cat_feature_count; ++i) {
    new (cat_features + i) string();
  }

  for (const auto &kv: features_map) {
    if (__builtin_expect(!kv.is_string_key(), false)) {
      continue;
    }
    const string &feature_name = kv.get_string_key();
    const uint64_t key_hash = string_hash(feature_name.c_str(), feature_name.size());
    double f_or_cat = kv.get_value();

    if (auto found_it = cbm.reindex_map_floats_and_cat.find(key_hash); found_it != cbm.reindex_map_floats_and_cat.end()) {
      int feature_id = found_it->second;
      if (feature_id >= CatboostModel::REINDEX_MAP_CATEGORIAL_SHIFT) {
        cat_features[feature_id - CatboostModel::REINDEX_MAP_CATEGORIAL_SHIFT] = f$strval(static_cast<int64_t>(std::round(f_or_cat)));
      } else {
        float_features[feature_id] = static_cast<float>(f_or_cat);
      }
    }
  }

  return predict_one<float>(cbm, float_features, cat_features, mutable_buffer);
}

array<double> kml_predict_catboost_by_vectors_multi(const kphp_ml::MLModel &kml,
                                                    const array<double> &float_features,
                                                    const array<string> &cat_features,
                                                    char *mutable_buffer) {
  const auto &cbm = std::get<CatboostModel>(kml.impl);

  if (!float_features.is_vector() || float_features.count() < cbm.float_feature_count) {
    php_warning("incorrect input size of float_features, model %s", kml.model_name.c_str());
    return {};
  }
  if (!cat_features.is_vector() || cat_features.count() < cbm.cat_feature_count) {
    php_warning("incorrect input size of cat_features, model %s", kml.model_name.c_str());
    return {};
  }

  return predict_one_multi<double>(cbm, float_features.get_const_vector_pointer(), cat_features.get_const_vector_pointer(), mutable_buffer);
}

array<double> kml_predict_catboost_by_ht_remap_str_keys_multi(const kphp_ml::MLModel &kml,
                                                              const array<double> &features_map,
                                                              char *mutable_buffer) {
  const auto &cbm = std::get<CatboostModel>(kml.impl);

  auto *float_features = reinterpret_cast<float *>(mutable_buffer);
  mutable_buffer += sizeof(float) * cbm.float_feature_count;
  std::fill_n(float_features, cbm.float_feature_count, 0.0);

  auto *cat_features = reinterpret_cast<string *>(mutable_buffer);
  mutable_buffer += sizeof(string) * cbm.cat_feature_count;
  for (int i = 0; i < cbm.cat_feature_count; ++i) {
    new (cat_features + i) string();
  }

  for (const auto &kv: features_map) {
    if (__builtin_expect(!kv.is_string_key(), false)) {
      continue;
    }
    const string &feature_name = kv.get_string_key();
    const uint64_t key_hash = string_hash(feature_name.c_str(), feature_name.size());
    double f_or_cat = kv.get_value();

    if (auto found_it = cbm.reindex_map_floats_and_cat.find(key_hash); found_it != cbm.reindex_map_floats_and_cat.end()) {
      int feature_id = found_it->second;
      if (feature_id >= CatboostModel::REINDEX_MAP_CATEGORIAL_SHIFT) {
        cat_features[feature_id - CatboostModel::REINDEX_MAP_CATEGORIAL_SHIFT] = f$strval(static_cast<int64_t>(std::round(f_or_cat)));
      } else {
        float_features[feature_id] = static_cast<float>(f_or_cat);
      }
    }
  }

  return predict_one_multi<float>(cbm, float_features, cat_features, mutable_buffer);
}

} // namespace kphp_ml_catboost
