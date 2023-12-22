#include "xgboost.h"

#include <functional>
#include <string>

struct XgbDensePredictor {
  struct MissingFloatPair {
    float at_vec_offset_0 = +1e+10;
    float at_vec_offset_1 = -1e+10;
  };
  float *vector_x{nullptr}; // assigned outside as a chunk in linear memory, 2 equal values per existing feature

  void fill_vector_x_ht_direct(const kphp_ml::XgbModel &xgb_model, const array<double> &features_map, bool skipping_zeroes) noexcept {
    for (const auto &iter : features_map) {
      const auto &feature_id = iter.get_key().as_int();
      const auto &fvalue = iter.get_value();
      if (feature_id < 0 || feature_id >= xgb_model.max_required_features) {
        continue;
      }
      if (skipping_zeroes && std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int vec_offset = xgb_model.offset_in_vec[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_str_key(const kphp_ml::XgbModel &xgb_model, const array<double> &features_map, bool skipping_zeroes) noexcept {
    for (const auto &iter : features_map) {
      const auto &feature_name = iter.get_key().as_string();
      const auto &fvalue = iter.get_value();
      if (skipping_zeroes && std::fabs(fvalue) < 1e-9) {
        continue;
      }

      auto found_it = xgb_model.reindex_map_str2int.find(std::hash<std::string>{}(std::string(feature_name.c_str())));
      if (found_it != xgb_model.reindex_map_str2int.end()) {
        int vec_offset = found_it->second;
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_int_key(const kphp_ml::XgbModel &xgb_model, const array<double> &features_map, bool skipping_zeroes) noexcept {

    for (const auto &iter : features_map) {
      const auto &feature_id = iter.get_key().as_int();
      const auto &fvalue = iter.get_value();
      if (feature_id < 0 || feature_id >= xgb_model.max_required_features) {
        continue;
      }
      if (skipping_zeroes && std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int vec_offset = xgb_model.reindex_map_int2int[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  float predict_one_tree(const kphp_ml::XgbTree &tree) const noexcept {
    const kphp_ml::XgbTreeNode *node = tree.nodes.data();
    while (!node->is_leaf()) {
      bool goto_right = vector_x[node->vec_offset_dense()] >= node->split_cond;
      node = &tree.nodes[node->left_child() + goto_right];
    }
    return node->split_cond;
  }
};

array<double> EvalXgboost::predict_input(const array<array<double>> &float_features) const {
  const auto &xgb_model = std::get<kphp_ml::XgbModel>(model.impl);

  const size_t rows_cnt = float_features.size().size;

  float *linear_memory = new float[kphp_ml::BATCH_SIZE_XGB * xgb_model.num_features_present * 2];
  XgbDensePredictor feat_vecs[kphp_ml::BATCH_SIZE_XGB];

  for (int i = 0; i < kphp_ml::BATCH_SIZE_XGB && i < rows_cnt; ++i) {
    feat_vecs[i].vector_x = linear_memory + i * xgb_model.num_features_present * 2;
  }

  const auto batches_cnt = static_cast<size_t>(std::ceil(static_cast<double>(rows_cnt) / kphp_ml::BATCH_SIZE_XGB));

  auto iter_done = float_features.begin();

  array<double> response;
  const float base_score = xgb_model.transform_base_score();
  response.reserve(rows_cnt, true);
  for (int i = 0; i < rows_cnt; ++i) {
    response.push_back(base_score);
  }

  for (int block_id = 0; block_id < batches_cnt; ++block_id) {
    const size_t batch_offset = block_id * kphp_ml::BATCH_SIZE_XGB;
    const size_t block_size = std::min(rows_cnt - batch_offset, kphp_ml::BATCH_SIZE_XGB);

    XgbDensePredictor::MissingFloatPair missing;
    std::fill((uint64_t *)linear_memory, reinterpret_cast<uint64_t *>(linear_memory) + block_size * xgb_model.num_features_present, *(uint64_t *)&missing);
    switch (model.input_kind) {
      case kphp_ml::InputKind::ht_remap_str_keys_to_fvalue:
        for (int i = 0; i < block_size; ++i) {
          feat_vecs[i].fill_vector_x_ht_remap_str_key(xgb_model, iter_done.get_value(), xgb_model.skip_zeroes);
          ++iter_done;
        }
        break;
      case kphp_ml::InputKind::ht_remap_int_keys_to_fvalue:
        for (int i = 0; i < block_size; ++i) {
          feat_vecs[i].fill_vector_x_ht_remap_int_key(xgb_model, iter_done.get_value(), xgb_model.skip_zeroes);
          ++iter_done;
        }
        break;
      case kphp_ml::InputKind::ht_direct_int_keys_to_fvalue:
        for (int i = 0; i < block_size; ++i) {
          feat_vecs[i].fill_vector_x_ht_direct(xgb_model, iter_done.get_value(), xgb_model.skip_zeroes);
          ++iter_done;
        }
        break;
      default:
        throw std::invalid_argument("unsupported input_kind for EvalXgboost");
    }
    for (const auto &tree : xgb_model.trees) {
      for (int i = 0; i < block_size; ++i) {
        int idx = batch_offset + i;
        response[idx] += feat_vecs[i].predict_one_tree(tree);
      }
    }
  }

  for (int i = 0; i < rows_cnt; ++i) {
    response[i] = xgb_model.transform_prediction(response[i]);
  }

  delete[] linear_memory;
  return response;
}