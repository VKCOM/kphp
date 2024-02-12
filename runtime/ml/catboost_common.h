#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace cb_common {

enum class CatboostModelCtrType { Borders, Buckets, BinarizedTargetMeanValue, FloatTargetMeanValue, Counter, FeatureFreq, CtrTypesCount };

struct CatboostModelCtr {
  uint64_t base_hash;
  CatboostModelCtrType base_ctr_type;
  int target_border_idx;
  float prior_num;
  float prior_denom;
  float shift;
  float scale;

  inline float calc(float count_in_class, float total_count) const noexcept {
    float ctr = (count_in_class + prior_num) / (total_count + prior_denom);
    return (ctr + shift) * scale;
  }

  inline float calc(int count_in_class, int total_count) const noexcept {
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

struct CatboostModelBase {
  int float_feature_count;
  int cat_feature_count;
  int binary_feature_count;
  int tree_count;
  std::vector<int> float_features_index;
  std::vector<std::vector<float>> float_feature_borders;
  std::vector<int> tree_depth;
  std::vector<int> cat_features_index;
  std::vector<int> one_hot_cat_feature_index;
  std::vector<std::vector<int>> one_hot_hash_values;
  std::vector<std::vector<float>> ctr_feature_borders;

  std::vector<float> leaf_values; // this and below are like a union
  std::vector<std::vector<float>> leaf_values_vec;

  double scale;

  double bias;                // this and below are like a union
  std::vector<double> biases; // this and below are like a union

  int dimension = -1;
  std::unordered_map<std::string, int> cat_features_hashes;

  CatboostModelCtrsContainer model_ctrs;
  // TODO
  // There are also embedded and text features
  // And we may want to add them later
};

struct CatboostModel : CatboostModelBase {
  std::vector<int> tree_split_feature_index;
  std::vector<unsigned char> tree_split_xor_mask;
  std::vector<unsigned char> tree_split_border;
};

struct AOSCatboostModel : public CatboostModelBase {
  struct Split {
    uint16_t feature_index;
    uint8_t xor_mask;
    uint8_t border;
  };
  std::vector<Split> tree_split;
  AOSCatboostModel() = default;
  explicit AOSCatboostModel(CatboostModel &&oth)
    : CatboostModelBase(std::move(static_cast<CatboostModelBase &&>(oth))) {
    tree_split.resize(oth.tree_split_border.size());
    for (size_t i = 0; i < oth.tree_split_border.size(); ++i) {
      tree_split[i].border = oth.tree_split_border[i];
      tree_split[i].feature_index = oth.tree_split_feature_index[i];
      tree_split[i].xor_mask = oth.tree_split_xor_mask[i];
    }
  }
};
} // namespace cb_common