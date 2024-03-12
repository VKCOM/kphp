#include "catboost.h"

#include <cassert>
#include <stdexcept>


#include "runtime/ml/interface.h"

using namespace cb_common;

static char *buffer;

struct FlatMap {
private:
  std::pair<int, int> *data;
  size_t size{0};
  size_t capacity;

public:
  explicit FlatMap(std::pair<int, int> *data, size_t capacity)
    : data(data)
    , capacity(capacity) {}

  void insert(int key, int value) {
    assert(size < capacity);
    data[size++] = std::pair{key, value};
  }

  int at(int key) {
    for (size_t i = 0; i < size; ++i) {
      if (data[i].first == key) {
        return data[i].second;
      }
    }
    throw std::out_of_range("No key \"" + std::to_string(key) + "\" in a map");
  }
};

static inline uint64_t calc_hash(uint64_t a, uint64_t b) {
  static constexpr uint64_t MAGIC_MULT = 0x4906ba494954cb65ULL;
  return MAGIC_MULT * (a + MAGIC_MULT * b);
}

static uint64_t calc_hashes(const unsigned char * binarized_features, int * hashed_cat_features,
                            const std::vector<int> &transposed_cat_feature_indexes,
                            const std::vector<CatboostBinFeatureIndexValue> &binarized_feature_indexes) {
  uint64_t result = 0;
  for (int cat_feature_index : transposed_cat_feature_indexes) {
    result = calc_hash(result, static_cast<uint64_t>(hashed_cat_features[cat_feature_index]));
  }
  for (const CatboostBinFeatureIndexValue &bin_feature_index : binarized_feature_indexes) {
    unsigned char binary_feature = binarized_features[bin_feature_index.bin_index];
    if (!bin_feature_index.check_value_equal) {
      result = calc_hash(result, static_cast<uint64_t>(binary_feature >= bin_feature_index.value));
    } else {
      result = calc_hash(result, static_cast<uint64_t>(binary_feature == bin_feature_index.value));
    }
  }
  return result;
}

static void calc_ctrs(const CatboostModelCtrsContainer &model_ctrs, unsigned char * binarized_features,
                      int * hashed_cat_features, float * result) {
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
        // todo do we need all cases in production?
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

static int get_hash(const std::string &cat_feature, const std::unordered_map<std::string, int> &cat_feature_hashes) {
  auto found_it = cat_feature_hashes.find(cat_feature);
  return found_it == cat_feature_hashes.end() ? 0x7fffffff : found_it->second;
}

static double eval(const AOSCatboostModel &model, const array<double> &float_features, const array<string> &cat_features) {
  assert(float_features.size().size == model.float_feature_count);
  assert(cat_features.size().size == model.cat_feature_count);

  // Binarise features
  auto *binary_features = reinterpret_cast<unsigned char *>(buffer);
  std::fill(binary_features, binary_features + model.binary_feature_count, 0);

  buffer += model.binary_feature_count * sizeof(unsigned char);
  buffer = reinterpret_cast<char *>(((ptrdiff_t)(buffer) + sizeof(int) - 1) / sizeof(int) * sizeof(int));

  int binary_feature_index = 0;

  // binarize float features
  for (int i = 0; i < model.float_feature_borders.size(); ++i) {
    for (double border : model.float_feature_borders[i]) {
      binary_features[binary_feature_index] += static_cast<unsigned char>((*float_features.find_value(model.float_features_index[i])) > border);
    }
    binary_feature_index++;
  }

  auto *transposed_hash = reinterpret_cast<int *>(buffer);
  buffer += model.cat_feature_count * sizeof(int);
  for (int i = 0; i < model.cat_feature_count; ++i) {
    const string *s = cat_features.find_value(i);
    if (s == nullptr) {
      transposed_hash[i] = 0x7fffffff;
    } else {
      transposed_hash[i] = get_hash(s->c_str(), model.cat_features_hashes); // todo cat_feature_hashes part of model?
    }
  }

  // binarize one hot cat features
  if (!model.one_hot_cat_feature_index.empty()) {
    FlatMap cat_feature_packed_indexes(reinterpret_cast<std::pair<int, int>*>(buffer), model.cat_feature_count);
    buffer += sizeof(std::pair<int, int>) * model.cat_feature_count;

    for (int i = 0; i < model.cat_feature_count; ++i) {
      cat_feature_packed_indexes.insert(model.cat_features_index[i],  i);
    }
    for (int i = 0; i < model.one_hot_cat_feature_index.size(); ++i) {
      int cat_idx = cat_feature_packed_indexes.at(model.one_hot_cat_feature_index[i]);
      int hash = transposed_hash[cat_idx];
      for (int border_idx = 0; border_idx < model.one_hot_hash_values[i].size(); ++border_idx) {
        binary_features[binary_feature_index] |= static_cast<unsigned char>(hash == model.one_hot_hash_values[i][border_idx]) * (border_idx + 1);
      }
      binary_feature_index++;
    }
  }

  // binarize ctr features
  if (model.model_ctrs.used_model_ctrs_count > 0) {
    auto * ctrs = reinterpret_cast<float *>(buffer);
    buffer += sizeof(float) * model.model_ctrs.used_model_ctrs_count;

    calc_ctrs(model.model_ctrs, binary_features, transposed_hash, ctrs);

    for (int i = 0; i < model.ctr_feature_borders.size(); ++i) {
      for (float border : model.ctr_feature_borders[i]) {
        binary_features[binary_feature_index] += static_cast<unsigned char>(ctrs[i] > border);
      }
      binary_feature_index++;
    }
  }

  // Extract and sum values from trees

  double result = 0.;
  int tree_splits_index = 0;
  int current_tree_leaf_values_index = 0;
  for (int tree_id = 0; tree_id < model.tree_count; ++tree_id) {
    int current_tree_depth = model.tree_depth[tree_id];
    int index = 0;
    for (int depth = 0; depth < current_tree_depth; ++depth) {
      auto border = model.tree_split[tree_splits_index + depth].border;
      auto feature_index = model.tree_split[tree_splits_index + depth].feature_index;
      auto xor_mask = model.tree_split[tree_splits_index + depth].xor_mask;
      index |= ((binary_features[feature_index] ^ xor_mask) >= border) << depth;
    }
    result += model.leaf_values[current_tree_leaf_values_index + index];
    tree_splits_index += current_tree_depth;
    current_tree_leaf_values_index += 1 << current_tree_depth;
  }
  return model.scale * result + model.bias;
}

static array<double> eval_multi(const AOSCatboostModel &model, const array<double> &float_features, const array<string> &cat_features) {
  assert(float_features.size().size == model.float_feature_count);
  assert(cat_features.size().size == model.cat_feature_count);

  // Binarise features
  auto *binary_features = reinterpret_cast<unsigned char *>(buffer);
  std::fill(binary_features, binary_features + model.binary_feature_count, 0);

  buffer += model.binary_feature_count * sizeof(unsigned char);
  buffer = reinterpret_cast<char *>(((ptrdiff_t)(buffer) + sizeof(int) - 1) / sizeof(int) * sizeof(int));

  int binary_feature_index = 0;

  // binarize float features
  for (int i = 0; i < model.float_feature_borders.size(); ++i) {
    for (double border : model.float_feature_borders[i]) {
      binary_features[binary_feature_index] += static_cast<unsigned char>((*float_features.find_value(model.float_features_index[i])) > border);
    }
    binary_feature_index++;
  }

  auto *transposed_hash = reinterpret_cast<int *>(buffer);
  buffer += model.cat_feature_count * sizeof(int);
  for (int i = 0; i < model.cat_feature_count; ++i) {
    const string *s = cat_features.find_value(i);
    if (s == nullptr) {
      transposed_hash[i] = 0x7fffffff;
    } else {
      transposed_hash[i] = get_hash(s->c_str(), model.cat_features_hashes); // todo cat_feature_hashes part of model?
    }
  }

  // binarize one hot cat features
  if (!model.one_hot_cat_feature_index.empty()) {
    FlatMap cat_feature_packed_indexes(reinterpret_cast<std::pair<int, int>*>(buffer), model.cat_feature_count);
    buffer += sizeof(std::pair<int, int>) * model.cat_feature_count;

    for (int i = 0; i < model.cat_feature_count; ++i) {
      cat_feature_packed_indexes.insert(model.cat_features_index[i],  i);
    }
    for (int i = 0; i < model.one_hot_cat_feature_index.size(); ++i) {
      int cat_idx = cat_feature_packed_indexes.at(model.one_hot_cat_feature_index[i]);
      int hash = transposed_hash[cat_idx];
      for (int border_idx = 0; border_idx < model.one_hot_hash_values[i].size(); ++border_idx) {
        binary_features[binary_feature_index] |= static_cast<unsigned char>(hash == model.one_hot_hash_values[i][border_idx]) * (border_idx + 1);
      }
      binary_feature_index++;
    }
  }

  // binarize ctr features
  if (model.model_ctrs.used_model_ctrs_count > 0) {
    auto * ctrs = reinterpret_cast<float *>(buffer);
    buffer += sizeof(float) * model.model_ctrs.used_model_ctrs_count;

    calc_ctrs(model.model_ctrs, binary_features, transposed_hash, ctrs);

    for (int i = 0; i < model.ctr_feature_borders.size(); ++i) {
      for (float border : model.ctr_feature_borders[i]) {
        binary_features[binary_feature_index] += static_cast<unsigned char>(ctrs[i] > border);
      }
      binary_feature_index++;
    }
  }

  // Extract and sum values from trees

  array<double> results(array_size(model.dimension, true));
  results.fill_vector(model.dimension, 0.0);
  const auto* leaf_values_ptr = model.leaf_values_vec.data();
  int tree_splits_index = 0;

  for (int tree_id = 0; tree_id < model.tree_count; ++tree_id) {
    int current_tree_depth = model.tree_depth[tree_id];
    int index = 0;
    for (int depth = 0; depth < current_tree_depth; ++depth) {
      auto border = model.tree_split[tree_splits_index + depth].border;
      auto feature_index = model.tree_split[tree_splits_index + depth].feature_index;
      auto xor_mask = model.tree_split[tree_splits_index + depth].xor_mask;
      index |= ((binary_features[feature_index] ^ xor_mask) >= border) << depth;
    }
    for (int i = 0; i < model.dimension; ++i) {
      results[i] += leaf_values_ptr[index][i];
    }
    tree_splits_index += current_tree_depth;
    leaf_values_ptr += (1 << current_tree_depth);
  }
  for (int i = 0; i < model.dimension; ++i) {
    results[i] = results[i] * model.scale + model.biases[i];
  }
  return results;


}

array<double> EvalCatboost::predict_input(const array<array<double>> &float_features, const array<array<string>> &cat_features) const {
  const auto &cbm = std::get<AOSCatboostModel>(model.impl);

  const auto [size, is_vec] = float_features.size();
  assert(is_vec);
  assert(cat_features.size().size == size && cat_features.size().is_vector == is_vec);

  array<double> resp;
  resp.reserve(size, true);
  assert(resp.is_vector());

  for (int row_id = 0; row_id < size; ++row_id) {
    buffer = PredictionBuffer;
    resp.emplace_back(eval(cbm, *float_features.find_value(row_id), *cat_features.find_value(row_id)));
  }
  return resp;
}

array<array<double>> EvalCatboost::predict_input_multi(const array<array<double>> &float_features, const array<array<string>> &cat_features) const {
  const auto &cbm = std::get<AOSCatboostModel>(model.impl);

  const auto [size, is_vec] = float_features.size();
  assert(is_vec);
  assert(cat_features.size().size == size && cat_features.size().is_vector == is_vec);

  array<array<double>> resp;
//  resp.reserve(size, true);
  assert(resp.is_vector());

  for (int row_id = 0; row_id < size; ++row_id) {
    buffer = PredictionBuffer;
    resp.emplace_back(eval_multi(cbm, *float_features.find_value(row_id), *cat_features.find_value(row_id)));
  }
  return resp;
}