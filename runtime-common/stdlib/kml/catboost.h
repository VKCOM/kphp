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
#include <optional>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

namespace kphp::kml::catboost {

enum class model_ctr_type {
  borders,
  buckets,
  binarized_target_mean_value,
  float_target_mean_value,
  counter,
  feature_freq,
  ctr_types_count,
};

struct model_ctr {
  uint64_t m_base_hash;
  model_ctr_type m_base_ctr_type;
  int32_t m_target_border_idx;
  float m_prior_num;
  float m_prior_denom;
  float m_shift;
  float m_scale;

  float calc(float count_in_class, float total_count) const noexcept {
    float ctr = (count_in_class + m_prior_num) / (total_count + m_prior_denom);
    return (ctr + m_shift) * m_scale;
  }

  float calc(int32_t count_in_class, int32_t total_count) const noexcept {
    return calc(static_cast<float>(count_in_class), static_cast<float>(total_count));
  }
};

struct bin_feature_index_value {
  int32_t m_bin_index;
  bool m_check_value_equal;
  unsigned char m_value;
};

struct ctr_mean_history {
  float m_sum;
  int32_t m_count;
};

template<template<class> class Allocator>
struct ctr_value_table {
  kphp::stl::unordered_map<uint64_t, uint32_t, Allocator> m_index_hash_viewer;
  int32_t m_target_classes_count;
  int32_t m_counter_denominator;
  kphp::stl::vector<ctr_mean_history, Allocator> m_ctr_mean_history;
  kphp::stl::vector<int32_t, Allocator> m_ctr_total;

  const uint32_t* resolve_hash_index(uint64_t hash) const noexcept {
    auto found_it = m_index_hash_viewer.find(hash);
    return found_it == m_index_hash_viewer.end() ? nullptr : &found_it->second;
  }
};

template<template<class> class Allocator>
struct ctr_data {
  kphp::stl::unordered_map<uint64_t, ctr_value_table<Allocator>, Allocator> m_learn_ctrs;
};

template<template<class> class Allocator>
struct projection {
  kphp::stl::vector<int32_t, Allocator> m_transposed_cat_feature_indexes;
  kphp::stl::vector<bin_feature_index_value, Allocator> m_binarized_indexes;
};

template<template<class> class Allocator>
struct compressed_model_ctr {
  projection<Allocator> m_projection;
  kphp::stl::vector<model_ctr, Allocator> m_model_ctrs;
};

template<template<class> class Allocator>
struct model_ctrs_container {
  int32_t m_used_model_ctrs_count{0};
  kphp::stl::vector<compressed_model_ctr<Allocator>, Allocator> m_compressed_model_ctrs;
  ctr_data<Allocator> m_ctr_data;
};

template<template<class> class Allocator>
struct model {
  struct split {
    uint16_t m_feature_index;
    uint8_t m_xor_mask;
    uint8_t m_border;
  };
  static_assert(sizeof(split) == 4);

  int32_t m_float_feature_count{};
  int32_t m_cat_feature_count{};
  int32_t m_binary_feature_count{};
  int32_t m_tree_count{};
  kphp::stl::vector<int32_t, Allocator> m_float_features_index;
  kphp::stl::vector<kphp::stl::vector<float, Allocator>, Allocator> m_float_feature_borders;
  kphp::stl::vector<int32_t, Allocator> m_tree_depth;
  kphp::stl::vector<int32_t, Allocator> m_one_hot_cat_feature_index;
  kphp::stl::vector<kphp::stl::vector<int32_t, Allocator>, Allocator> m_one_hot_hash_values;
  kphp::stl::vector<kphp::stl::vector<float, Allocator>, Allocator> m_ctr_feature_borders;

  kphp::stl::vector<split, Allocator> m_tree_split;
  kphp::stl::vector<float, Allocator> m_leaf_values; // this and below are like a union
  kphp::stl::vector<kphp::stl::vector<float, Allocator>, Allocator> m_leaf_values_vec;

  double m_scale{};

  double m_bias{}; // this and below are like a union
  kphp::stl::vector<double, Allocator> m_biases{};

  int32_t m_dimension = -1; // absent in case of NON-multiclass classification
  kphp::stl::unordered_map<uint64_t, int32_t, Allocator> m_cat_features_hashes{};

  model_ctrs_container<Allocator> m_model_ctrs{};
  // todo there are also embedded and text features, we may want to add them later

  // to accept input_kind = ht_remap_str_keys_to_fvalue_or_catstr and similar
  // 1) we store [hash => vec_idx] instead of [feature_name => vec_idx], we don't expect collisions
  // 2) this reindex_map contains both reindexes of float and categorial features, but categorial are large:
  //    [ 'emb_7' => 7, ..., 'user_age_group' => 1000001, 'user_os' => 1000002 ]
  //    the purpose of storing two maps in one is to use a single hashtable lookup when remapping
  // todo this ht is filled once (on .kml loading) and used many-many times for lookup; maybe, find smth faster than std
  kphp::stl::unordered_map<uint64_t, int32_t, Allocator> m_reindex_map_floats_and_cat{};
  static constexpr int32_t REINDEX_MAP_CATEGORIAL_SHIFT = 1000000;

private:
  template<class KMLReader, template<class> class ProjectionAllocator>
  void read_field(KMLReader& kml_reader, projection<ProjectionAllocator>& v) noexcept {
    kml_reader.read_vec(v.m_transposed_cat_feature_indexes);
    kml_reader.read_vec(v.m_binarized_indexes);
  }

  template<class KMLReader>
  void read_field(KMLReader& kml_reader, model_ctr& v) noexcept {
    kml_reader.read_uint64(v.m_base_hash);
    kml_reader.read_enum(v.m_base_ctr_type);
    kml_reader.read_int32(v.m_target_border_idx);
    kml_reader.read_float(v.m_prior_num);
    kml_reader.read_float(v.m_prior_denom);
    kml_reader.read_float(v.m_shift);
    kml_reader.read_float(v.m_scale);
  }

  template<class KMLReader, template<class> class CompressedModelCtrAllocator>
  void read_field(KMLReader& kml_reader, compressed_model_ctr<CompressedModelCtrAllocator>& v) noexcept {
    read_field(kml_reader, v.m_projection);

    auto sz = kml_reader.read_int32();
    v.m_model_ctrs.resize(sz);
    for (auto& item : v.m_model_ctrs) {
      read_field(kml_reader, item);
    }
  }

  template<class KMLReader, template<class> class CtrValueTableAllocator>
  void read_field(KMLReader& kml_reader, ctr_value_table<CtrValueTableAllocator>& v) noexcept {
    auto sz = kml_reader.read_int32();
    v.m_index_hash_viewer.reserve(sz);
    for (auto i = 0; i < sz; ++i) {
      uint64_t k = 0;
      kml_reader.read_uint64(k);
      kml_reader.read_uint32(v.m_index_hash_viewer[k]);
    }

    kml_reader.read_int32(v.m_target_classes_count);
    kml_reader.read_int32(v.m_counter_denominator);
    kml_reader.read_vec(v.m_ctr_mean_history);
    kml_reader.read_vec(v.m_ctr_total);
  }

  template<class KMLReader, template<class> class CtrDataAllocator>
  void write_field(KMLReader& kml_reader, ctr_data<CtrDataAllocator>& v) noexcept {
    auto sz = kml_reader.read_int32();
    v.m_learn_ctrs.reserve(sz);
    for (auto i = 0; i < sz; ++i) {
      uint64_t key = 0;
      kml_reader.read_uint64(key);
      read_field(kml_reader, v.m_learn_ctrs[key]);
    }
  }

  template<class KMLReader, template<class> class ModelCtrsContainerAllocator>
  void read_field(KMLReader& kml_reader, model_ctrs_container<ModelCtrsContainerAllocator>& v) noexcept {
    bool has = false;
    kml_reader.read_bool(has);

    if (has) {
      kml_reader.read_int32(v.m_used_model_ctrs_count);

      auto sz = kml_reader.read_int32();
      v.m_compressed_model_ctrs.resize(sz);
      for (auto& item : v.m_compressed_model_ctrs) {
        read_field(kml_reader, item);
      }

      write_field(kml_reader, v.m_ctr_data);
    }
  }

public:
  size_t mutable_buffer_size() const noexcept {
    return m_cat_feature_count * sizeof(string) + m_float_feature_count * sizeof(float) +
           static_cast<size_t>((m_binary_feature_count + 4 - 1) / 4 * 4) + // round up to 4 bytes
           m_cat_feature_count * sizeof(int32_t) + m_model_ctrs.m_used_model_ctrs_count * sizeof(float);
  }

  bool is_multi_classification() const noexcept {
    return m_dimension != -1 && !m_leaf_values_vec.empty();
  }

  template<class KMLReader>
  static std::optional<model<Allocator>> load(KMLReader& kml_reader) noexcept {
    model<Allocator> cbm{};

    kml_reader.read_int32(cbm.m_float_feature_count);
    kml_reader.read_int32(cbm.m_cat_feature_count);
    kml_reader.read_int32(cbm.m_binary_feature_count);
    kml_reader.read_int32(cbm.m_tree_count);

    if (kml_reader.is_eof()) {
      php_warning("failed to load CatBoost model: unexpected EOF");
      return std::nullopt;
    }

    kml_reader.read_vec(cbm.m_float_features_index);
    kml_reader.read_2d_vec(cbm.m_float_feature_borders);
    kml_reader.read_vec(cbm.m_tree_depth);
    kml_reader.read_vec(cbm.m_one_hot_cat_feature_index);
    kml_reader.read_2d_vec(cbm.m_one_hot_hash_values);
    kml_reader.read_2d_vec(cbm.m_ctr_feature_borders);

    if (kml_reader.is_eof()) {
      php_warning("failed to load CatBoost model: unexpected EOF");
      return std::nullopt;
    }

    kml_reader.read_vec(cbm.m_tree_split);
    kml_reader.read_vec(cbm.m_leaf_values);
    kml_reader.read_2d_vec(cbm.m_leaf_values_vec);

    if (kml_reader.is_eof()) {
      php_warning("failed to load CatBoost model: unexpected EOF");
      return std::nullopt;
    }

    kml_reader.read_double(cbm.m_scale);
    kml_reader.read_double(cbm.m_bias);
    kml_reader.read_vec(cbm.m_biases);
    kml_reader.read_int32(cbm.m_dimension);

    if (kml_reader.is_eof()) {
      php_warning("failed to load CatBoost model: unexpected EOF");
      return std::nullopt;
    }

    auto cat_hashes_sz = kml_reader.read_int32();
    cbm.m_cat_features_hashes.reserve(cat_hashes_sz);
    for (auto i = 0; i < cat_hashes_sz; ++i) {
      uint64_t key_hash = 0;
      kml_reader.read_uint64(key_hash);
      kml_reader.read_int32(cbm.m_cat_features_hashes[key_hash]);
    }

    if (kml_reader.is_eof()) {
      php_warning("failed to load CatBoost model: unexpected EOF");
      return std::nullopt;
    }

    cbm.read_field(kml_reader, cbm.m_model_ctrs);

    auto reindex_sz = kml_reader.read_int32();
    cbm.m_reindex_map_floats_and_cat.reserve(reindex_sz);
    for (auto i = 0; i < reindex_sz; ++i) {
      uint64_t hash = 0;
      kml_reader.read_uint64(hash);
      kml_reader.read_int32(cbm.m_reindex_map_floats_and_cat[hash]);
    }

    return cbm;
  }
};

namespace detail {

inline uint64_t calc_hash(uint64_t a, uint64_t b) noexcept {
  static constexpr uint64_t MAGIC_MULT = 0x4906ba494954cb65ULL;
  return MAGIC_MULT * (a + MAGIC_MULT * b);
}

template<template<class> class Allocator>
uint64_t calc_hashes(const unsigned char* binarized_features, const int32_t* hashed_cat_features,
                     const kphp::stl::vector<int32_t, Allocator>& transposed_cat_feature_indexes,
                     const kphp::stl::vector<bin_feature_index_value, Allocator>& binarized_feature_indexes) noexcept {
  uint64_t result = 0;
  for (auto cat_feature_index : transposed_cat_feature_indexes) {
    result = calc_hash(result, static_cast<uint64_t>(hashed_cat_features[cat_feature_index]));
  }
  for (const auto& bin_feature_index : binarized_feature_indexes) {
    unsigned char binary_feature = binarized_features[bin_feature_index.m_bin_index];
    if (!bin_feature_index.m_check_value_equal) {
      result = calc_hash(result, binary_feature >= bin_feature_index.m_value);
    } else {
      result = calc_hash(result, binary_feature == bin_feature_index.m_value);
    }
  }
  return result;
}

template<template<class> class Allocator>
void calc_ctrs(const model_ctrs_container<Allocator>& model_ctrs, const unsigned char* binarized_features, const int32_t* hashed_cat_features,
               float* result) noexcept {
  int32_t result_index = 0;

  for (size_t i = 0; i < model_ctrs.m_compressed_model_ctrs.size(); ++i) {
    const auto& proj = model_ctrs.m_compressed_model_ctrs[i].m_projection;
    uint64_t ctr_hash = calc_hashes(binarized_features, hashed_cat_features, proj.m_transposed_cat_feature_indexes, proj.m_binarized_indexes);

    for (size_t j = 0; j < model_ctrs.m_compressed_model_ctrs[i].m_model_ctrs.size(); ++j, ++result_index) {
      const model_ctr& ctr = model_ctrs.m_compressed_model_ctrs[i].m_model_ctrs[j];
      const auto& learn_ctr = model_ctrs.m_ctr_data.m_learn_ctrs.at(ctr.m_base_hash);
      auto ctr_type = ctr.m_base_ctr_type;
      const auto* bucket_ptr = learn_ctr.resolve_hash_index(ctr_hash);
      if (bucket_ptr == nullptr) {
        result[result_index] = ctr.calc(0, 0);
      } else {
        uint32_t bucket = *bucket_ptr;
        if (ctr_type == model_ctr_type::binarized_target_mean_value || ctr_type == model_ctr_type::float_target_mean_value) {
          const ctr_mean_history& ctr_mean_history = learn_ctr.m_ctr_mean_history[bucket];
          result[result_index] = ctr.calc(ctr_mean_history.m_sum, static_cast<float>(ctr_mean_history.m_count));
        } else if (ctr_type == model_ctr_type::counter || ctr_type == model_ctr_type::feature_freq) {
          const auto& ctr_total = learn_ctr.m_ctr_total;
          result[result_index] = ctr.calc(ctr_total[bucket], learn_ctr.m_counter_denominator);
        } else if (ctr_type == model_ctr_type::buckets) {
          const auto& ctr_history = learn_ctr.m_ctr_total;
          int32_t target_classes_count = learn_ctr.m_target_classes_count;
          int32_t good_count = ctr_history[bucket * target_classes_count + ctr.m_target_border_idx];
          int32_t total_count = 0;
          for (auto classId = 0; classId < target_classes_count; ++classId) {
            total_count += ctr_history[bucket * target_classes_count + classId];
          }
          result[result_index] = ctr.calc(good_count, total_count);
        } else {
          const auto& ctr_history = learn_ctr.m_ctr_total;
          int32_t target_classes_count = learn_ctr.m_target_classes_count;
          if (target_classes_count > 2) {
            int32_t good_count = 0;
            int32_t total_count = 0;
            for (auto class_id = 0; class_id < ctr.m_target_border_idx + 1; ++class_id) {
              total_count += ctr_history[bucket * target_classes_count + class_id];
            }
            for (auto class_id = ctr.m_target_border_idx + 1; class_id < target_classes_count; ++class_id) {
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

template<template<class> class Allocator>
int32_t get_hash(const string& cat_feature, const kphp::stl::unordered_map<uint64_t, int32_t, Allocator>& cat_feature_hashes) noexcept {
  auto found_it = cat_feature_hashes.find(string_hash(cat_feature.c_str(), cat_feature.size()));
  return found_it == cat_feature_hashes.end() ? 0x7fffffff : found_it->second;
}

template<class FloatOrDouble, template<class> class Allocator>
double predict_one(const kphp::kml::catboost::model<Allocator>& cbm, const FloatOrDouble* float_features, const string* cat_features,
                   std::byte* mutable_buffer) noexcept {
  std::byte* p_buffer = mutable_buffer;

  // Binarise features
  auto* binary_features = reinterpret_cast<unsigned char*>(mutable_buffer);
  p_buffer += (cbm.m_binary_feature_count + 4 - 1) / 4 * 4; // round up to 4 bytes

  int32_t binary_feature_index = -1;

  // binarize float features
  for (size_t i = 0; i < cbm.m_float_feature_borders.size(); ++i) {
    int32_t cur = 0;
    for (double border : cbm.m_float_feature_borders[i]) {
      cur += float_features[cbm.m_float_features_index[i]] > border;
    }
    binary_features[++binary_feature_index] = cur;
  }

  auto* transposed_hash = reinterpret_cast<int32_t*>(p_buffer);
  p_buffer += cbm.m_cat_feature_count * sizeof(int32_t);
  for (auto i = 0; i < cbm.m_cat_feature_count; ++i) {
    transposed_hash[i] = get_hash(cat_features[i], cbm.m_cat_features_hashes);
  }

  // binarize one hot cat features
  // note, that it slightly differs from original: one_hot_cat_feature_index is precomputed in converting to kml, no repack needed
  for (size_t i = 0; i < cbm.m_one_hot_cat_feature_index.size(); ++i) {
    int32_t cat_idx = cbm.m_one_hot_cat_feature_index[i];
    int32_t hash = transposed_hash[cat_idx];
    int32_t cur = 0;
    for (size_t border_idx = 0; border_idx < cbm.m_one_hot_hash_values[i].size(); ++border_idx) {
      cur |= (hash == cbm.m_one_hot_hash_values[i][border_idx]) * (border_idx + 1);
    }
    binary_features[++binary_feature_index] = cur;
  }

  // binarize ctr features
  if (cbm.m_model_ctrs.m_used_model_ctrs_count > 0) {
    auto* ctrs = reinterpret_cast<float*>(p_buffer);
    p_buffer += cbm.m_model_ctrs.m_used_model_ctrs_count * sizeof(float);
    calc_ctrs(cbm.m_model_ctrs, binary_features, transposed_hash, ctrs);

    for (size_t i = 0; i < cbm.m_ctr_feature_borders.size(); ++i) {
      unsigned char cur = 0;
      for (float border : cbm.m_ctr_feature_borders[i]) {
        cur += ctrs[i] > border;
      }
      binary_features[++binary_feature_index] = cur;
    }
  }

  // Extract and sum values from trees

  double result = 0.;
  int32_t tree_splits_index = 0;
  int32_t current_tree_leaf_values_index = 0;

  for (auto tree_id = 0; tree_id < cbm.m_tree_count; ++tree_id) {
    int32_t current_tree_depth = cbm.m_tree_depth[tree_id];
    int32_t index = 0;
    for (auto depth = 0; depth < current_tree_depth; ++depth) {
      const auto& split = cbm.m_tree_split[tree_splits_index + depth];
      index |= ((binary_features[split.m_feature_index] ^ split.m_xor_mask) >= split.m_border) << depth;
    }
    result += cbm.m_leaf_values[current_tree_leaf_values_index + index];
    tree_splits_index += current_tree_depth;
    current_tree_leaf_values_index += 1 << current_tree_depth;
  }

  return cbm.m_scale * result + cbm.m_bias;
}

template<class FloatOrDouble, template<class> class Allocator>
array<double> predict_one_multi(const kphp::kml::catboost::model<Allocator>& cbm, const FloatOrDouble* float_features, const string* cat_features,
                                std::byte* mutable_buffer) noexcept {
  std::byte* p_buffer = mutable_buffer;

  // Binarise features
  auto* binary_features = reinterpret_cast<unsigned char*>(mutable_buffer);
  p_buffer += (cbm.m_binary_feature_count + 4 - 1) / 4 * 4; // round up to 4 bytes

  int32_t binary_feature_index = -1;

  // binarize float features
  for (size_t i = 0; i < cbm.m_float_feature_borders.size(); ++i) {
    int32_t cur = 0;
    for (double border : cbm.m_float_feature_borders[i]) {
      cur += float_features[cbm.m_float_features_index[i]] > border;
    }
    binary_features[++binary_feature_index] = cur;
  }

  auto* transposed_hash = reinterpret_cast<int32_t*>(p_buffer);
  p_buffer += cbm.m_cat_feature_count * sizeof(int32_t);
  for (auto i = 0; i < cbm.m_cat_feature_count; ++i) {
    transposed_hash[i] = get_hash(cat_features[i], cbm.m_cat_features_hashes);
  }

  // binarize one hot cat features
  // note, that it slightly differs from original: one_hot_cat_feature_index is precomputed in converting to kml, no repack needed
  for (size_t i = 0; i < cbm.m_one_hot_cat_feature_index.size(); ++i) {
    int32_t cat_idx = cbm.m_one_hot_cat_feature_index[i];
    int32_t hash = transposed_hash[cat_idx];
    int32_t cur = 0;
    for (size_t border_idx = 0; border_idx < cbm.m_one_hot_hash_values[i].size(); ++border_idx) {
      cur |= (hash == cbm.m_one_hot_hash_values[i][border_idx]) * (border_idx + 1);
    }
    binary_features[++binary_feature_index] = cur;
  }

  // binarize ctr features
  if (cbm.m_model_ctrs.m_used_model_ctrs_count > 0) {
    auto* ctrs = reinterpret_cast<float*>(p_buffer);
    p_buffer += cbm.m_model_ctrs.m_used_model_ctrs_count * sizeof(float);
    calc_ctrs(cbm.m_model_ctrs, binary_features, transposed_hash, ctrs);

    for (size_t i = 0; i < cbm.m_ctr_feature_borders.size(); ++i) {
      int32_t cur = 0;
      for (float border : cbm.m_ctr_feature_borders[i]) {
        cur += ctrs[i] > border;
      }
      binary_features[++binary_feature_index] = cur;
    }
  }

  // Extract and sum values from trees

  array<double> results(array_size(cbm.m_dimension, true));
  for (auto i = 0; i < cbm.m_dimension; ++i) {
    results[i] = 0.0;
  }

  const auto* leaf_values_ptr = cbm.m_leaf_values_vec.data();
  int32_t tree_ptr = 0;

  for (auto tree_id = 0; tree_id < cbm.m_tree_count; ++tree_id) {
    int32_t current_tree_depth = cbm.m_tree_depth[tree_id];
    int32_t index = 0;
    for (auto depth = 0; depth < current_tree_depth; ++depth) {
      const auto& split = cbm.m_tree_split[tree_ptr + depth];
      index |= ((binary_features[split.m_feature_index] ^ split.m_xor_mask) >= split.m_border) << depth;
    }
    for (auto i = 0; i < cbm.m_dimension; ++i) {
      results[i] += leaf_values_ptr[index][i];
    }
    tree_ptr += current_tree_depth;
    leaf_values_ptr += 1 << current_tree_depth;
  }
  for (auto i = 0; i < cbm.m_dimension; ++i) {
    results[i] = results[i] * cbm.m_scale + cbm.m_biases[i];
  }

  return results;
}

} // namespace detail

template<template<class> class Allocator>
double predict_by_vectors(const kphp::kml::catboost::model<Allocator>& cbm, std::string_view model_name, const array<double>& float_features,
                          const array<string>& cat_features, std::byte* mutable_buffer) noexcept {
  if (!float_features.is_vector() || float_features.count() < cbm.m_float_feature_count) {
    php_warning("incorrect input size for float_features, model %s", model_name.data());
    return 0.0;
  }
  if (!cat_features.is_vector() || cat_features.count() < cbm.m_cat_feature_count) {
    php_warning("incorrect input size for cat_features, model %s", model_name.data());
    return 0.0;
  }

  return detail::predict_one<double>(cbm, float_features.get_const_vector_pointer(), cat_features.get_const_vector_pointer(), mutable_buffer);
}

template<template<class> class Allocator>
array<double> predict_by_vectors_multi(const kphp::kml::catboost::model<Allocator>& cbm, std::string_view model_name, const array<double>& float_features,
                                       const array<string>& cat_features, std::byte* mutable_buffer) noexcept {
  if (!float_features.is_vector() || float_features.count() < cbm.m_float_feature_count) {
    php_warning("incorrect input size of float_features, model %s", model_name.data());
    return {};
  }
  if (!cat_features.is_vector() || cat_features.count() < cbm.m_cat_feature_count) {
    php_warning("incorrect input size of cat_features, model %s", model_name.data());
    return {};
  }

  return detail::predict_one_multi<double>(cbm, float_features.get_const_vector_pointer(), cat_features.get_const_vector_pointer(), mutable_buffer);
}

template<template<class> class Allocator>
double predict_by_ht_remap_str_keys(const kphp::kml::catboost::model<Allocator>& cbm, const array<double>& features_map, std::byte* mutable_buffer) noexcept {
  auto* float_features = reinterpret_cast<float*>(mutable_buffer);
  mutable_buffer += sizeof(float) * cbm.m_float_feature_count;
  std::fill_n(float_features, cbm.m_float_feature_count, 0.0);

  auto* cat_features = reinterpret_cast<string*>(mutable_buffer);
  mutable_buffer += sizeof(string) * cbm.m_cat_feature_count;
  for (auto i = 0; i < cbm.m_cat_feature_count; ++i) {
    new (cat_features + i) string();
  }

  for (const auto& kv : features_map) {
    if (__builtin_expect(!kv.is_string_key(), false)) {
      continue;
    }
    const string& feature_name = kv.get_string_key();
    const uint64_t key_hash = string_hash(feature_name.c_str(), feature_name.size());
    double f_or_cat = kv.get_value();

    if (auto found_it = cbm.m_reindex_map_floats_and_cat.find(key_hash); found_it != cbm.m_reindex_map_floats_and_cat.end()) {
      int32_t feature_id = found_it->second;
      if (feature_id >= kphp::kml::catboost::model<Allocator>::REINDEX_MAP_CATEGORIAL_SHIFT) {
        cat_features[feature_id - kphp::kml::catboost::model<Allocator>::REINDEX_MAP_CATEGORIAL_SHIFT] = f$strval(static_cast<int64_t>(std::round(f_or_cat)));
      } else {
        float_features[feature_id] = static_cast<float>(f_or_cat);
      }
    }
  }

  return detail::predict_one<float>(cbm, float_features, cat_features, mutable_buffer);
}

template<template<class> class Allocator>
array<double> predict_by_ht_remap_str_keys_multi(const kphp::kml::catboost::model<Allocator>& cbm, const array<double>& features_map,
                                                 std::byte* mutable_buffer) noexcept {
  auto* float_features = reinterpret_cast<float*>(mutable_buffer);
  mutable_buffer += sizeof(float) * cbm.m_float_feature_count;
  std::fill_n(float_features, cbm.m_float_feature_count, 0.0);

  auto* cat_features = reinterpret_cast<string*>(mutable_buffer);
  mutable_buffer += sizeof(string) * cbm.m_cat_feature_count;
  for (auto i = 0; i < cbm.m_cat_feature_count; ++i) {
    new (cat_features + i) string{};
  }

  for (const auto& kv : features_map) {
    if (__builtin_expect(!kv.is_string_key(), false)) {
      continue;
    }
    const string& feature_name = kv.get_string_key();
    const uint64_t key_hash = string_hash(feature_name.c_str(), feature_name.size());
    double f_or_cat = kv.get_value();

    if (auto found_it = cbm.m_reindex_map_floats_and_cat.find(key_hash); found_it != cbm.m_reindex_map_floats_and_cat.end()) {
      int32_t feature_id = found_it->second;
      if (feature_id >= kphp::kml::catboost::model<Allocator>::REINDEX_MAP_CATEGORIAL_SHIFT) {
        cat_features[feature_id - kphp::kml::catboost::model<Allocator>::REINDEX_MAP_CATEGORIAL_SHIFT] = f$strval(static_cast<int64_t>(std::round(f_or_cat)));
      } else {
        float_features[feature_id] = static_cast<float>(f_or_cat);
      }
    }
  }

  return detail::predict_one_multi<float>(cbm, float_features, cat_features, mutable_buffer);
}

} // namespace kphp::kml::catboost
