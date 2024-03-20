#include "kphp_ml/kphp_ml.h"
#include "kphp_ml/kphp_ml_xgboost.h"

#include <cmath>

#include "utils/string_hash.h"
#include "testing_infra/infra_cpp.h"

namespace kphp_ml_xgboost {

static_assert(sizeof(XgbTreeNode) == 8, "unexpected sizeof(XgbTreeNode)");

struct XgbDensePredictor {
  struct MissingFloatPair {
    float at_vec_offset_0 = +1e+10;
    float at_vec_offset_1 = -1e+10;
  };

  float *vector_x{nullptr}; // assigned outside as a chunk in linear memory, 2 equal values per existing feature

  typedef void (XgbDensePredictor::*filler_int_keys)(const XgboostModel &xgb, const std::unordered_map<int, double> &features_map) const noexcept;
  typedef void (XgbDensePredictor::*filler_str_keys)(const XgboostModel &xgb, const std::unordered_map<std::string, double> &features_map) const noexcept;

  void fill_vector_x_ht_direct(const XgboostModel &xgb, const std::unordered_map<int, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const int feature_id = kv.first;
      const double fvalue = kv.second;

      if (feature_id < 0 || feature_id >= xgb.max_required_features) {  // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }

      int vec_offset = xgb.offset_in_vec[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_direct_sz(const XgboostModel &xgb, const std::unordered_map<int, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const int feature_id = kv.first;
      const double fvalue = kv.second;

      if (feature_id < 0 || feature_id >= xgb.max_required_features) {  // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }
      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int vec_offset = xgb.offset_in_vec[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_str_key(const XgboostModel &xgb, const std::unordered_map<std::string, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const std::string &feature_name = kv.first;
      const double fvalue = kv.second;

      auto found_it = xgb.reindex_map_str2int.find(string_hash(feature_name.c_str(), feature_name.size()));
      if (found_it != xgb.reindex_map_str2int.end()) {  // input contains [ "unexisting_feature" => 0.123 ], it's ok
        int vec_offset = found_it->second;
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_str_key_sz(const XgboostModel &xgb, const std::unordered_map<std::string, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const std::string &feature_name = kv.first;
      const double fvalue = kv.second;

      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      auto found_it = xgb.reindex_map_str2int.find(string_hash(feature_name.c_str(), feature_name.size()));
      if (found_it != xgb.reindex_map_str2int.end()) {  // input contains [ "unexisting_feature" => 0.123 ], it's ok
        int vec_offset = found_it->second;
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_int_key(const XgboostModel &xgb, const std::unordered_map<int, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const int feature_id = kv.first;
      const double fvalue = kv.second;

      if (feature_id < 0 || feature_id >= xgb.max_required_features) {  // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }

      int vec_offset = xgb.reindex_map_int2int[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  void fill_vector_x_ht_remap_int_key_sz(const XgboostModel &xgb, const std::unordered_map<int, double> &features_map) const noexcept {
    for (const auto &kv: features_map) {
      const int feature_id = kv.first;
      const double fvalue = kv.second;

      if (feature_id < 0 || feature_id >= xgb.max_required_features) {  // unexisting index [ 100000 => 0.123 ], it's ok
        continue;
      }
      if (std::fabs(fvalue) < 1e-9) {
        continue;
      }

      int vec_offset = xgb.reindex_map_int2int[feature_id];
      if (vec_offset != -1) {
        vector_x[vec_offset] = static_cast<float>(fvalue);
        vector_x[vec_offset + 1] = static_cast<float>(fvalue);
      }
    }
  }

  float predict_one_tree(const XgbTree &tree) const noexcept {
    const XgbTreeNode *node = tree.nodes.data();
    while (!node->is_leaf()) {
      bool goto_right = vector_x[node->vec_offset_dense()] >= node->split_cond;
      node = &tree.nodes[node->left_child() + goto_right];
    }
    return node->split_cond;
  }
};

[[gnu::always_inline]] static inline float transform_base_score(XGTrainParamObjective tparam_objective, float base_score) {
  switch (tparam_objective) {
    case XGTrainParamObjective::binary_logistic:
      return -logf(1.0f / base_score - 1.0f);
    case XGTrainParamObjective::rank_pairwise:
      return base_score;
    default:
      __builtin_unreachable();
  }
}

[[gnu::always_inline]] static inline double transform_prediction(XGTrainParamObjective tparam_objective, double score) {
  switch (tparam_objective) {
    case XGTrainParamObjective::binary_logistic:
      return 1.0 / (1.0 + std::exp(-score));
    case XGTrainParamObjective::rank_pairwise:
      return score;
    default:
      __builtin_unreachable();
  }
}

float XgboostModel::transform_base_score() const noexcept {
  return kphp_ml_xgboost::transform_base_score(tparam_objective, base_score);
}

double XgboostModel::transform_prediction(double score) const noexcept {
  score = kphp_ml_xgboost::transform_prediction(tparam_objective, score);

  switch (calibration.calibration_method) {
    case CalibrationMethod::platt_scaling:
      if (tparam_objective == XGTrainParamObjective::binary_logistic) {
        score = -log(1. / score - 1);
      }
      return 1 / (1. + exp(-(calibration.platt_slope * score + calibration.platt_intercept)));
    default:
      return score;
  }
}

std::vector<double> kml_predict_xgboost(const kphp_ml::MLModel &kml,
                         const std::vector<InputRow> &in,
                         char *mutable_buffer) {
  const auto &xgb = std::get<XgboostModel>(kml.impl);
  int n_rows = static_cast<int>(in.size());

  XgbDensePredictor::filler_int_keys filler_int_keys{nullptr};
  XgbDensePredictor::filler_str_keys filler_str_keys{nullptr};
  switch (kml.input_kind) {
    case kphp_ml::InputKind::ht_direct_int_keys_to_fvalue:
      filler_int_keys = xgb.skip_zeroes ? &XgbDensePredictor::fill_vector_x_ht_direct_sz : &XgbDensePredictor::fill_vector_x_ht_direct;
      break;
    case kphp_ml::InputKind::ht_remap_int_keys_to_fvalue:
      filler_int_keys = xgb.skip_zeroes ? &XgbDensePredictor::fill_vector_x_ht_remap_int_key_sz : &XgbDensePredictor::fill_vector_x_ht_remap_int_key;
      break;
    case kphp_ml::InputKind::ht_remap_str_keys_to_fvalue:
      filler_str_keys = xgb.skip_zeroes ? &XgbDensePredictor::fill_vector_x_ht_remap_str_key_sz : &XgbDensePredictor::fill_vector_x_ht_remap_str_key;
      break;
    default:
      assert(0 && "invalid input_kind");
      return {};
  }

  XgbDensePredictor feat_vecs[BATCH_SIZE_XGB];
  for (int i = 0; i < BATCH_SIZE_XGB && i < n_rows; ++i) {
    feat_vecs[i].vector_x = reinterpret_cast<float *>(mutable_buffer) + i * xgb.num_features_present * 2;
  }
  auto iter_done = in.begin();

  const float base_score = xgb.transform_base_score();
  std::vector<double> out_predictions;
  out_predictions.reserve(n_rows);
  for (int i = 0; i < n_rows; ++i) {
    out_predictions.push_back(base_score);
  }

  const int n_total = static_cast<int>(out_predictions.size());
  const int n_batches = std::ceil(static_cast<double>(n_total) / BATCH_SIZE_XGB);

  for (int block_id = 0; block_id < n_batches; ++block_id) {
    const int batch_offset = block_id * BATCH_SIZE_XGB;
    const int block_size = std::min(n_total - batch_offset, BATCH_SIZE_XGB);

    XgbDensePredictor::MissingFloatPair missing;
    std::fill_n(reinterpret_cast<uint64_t *>(mutable_buffer), block_size * xgb.num_features_present, *reinterpret_cast<uint64_t *>(&missing));
    if (filler_int_keys != nullptr) {
      for (int i = 0; i < block_size; ++i) {
        (feat_vecs[i].*filler_int_keys)(xgb, iter_done->ht_int_keys_to_fvalue);
        ++iter_done;
      }
    } else {
      for (int i = 0; i < block_size; ++i) {
        (feat_vecs[i].*filler_str_keys)(xgb, iter_done->ht_str_keys_to_fvalue);
        ++iter_done;
      }
    }

    for (const XgbTree &tree: xgb.trees) {
      for (int i = 0; i < block_size; ++i) {
        out_predictions[batch_offset + i] += feat_vecs[i].predict_one_tree(tree);
      }
    }
  }

  for (int i = 0; i < n_rows; ++i) {
    out_predictions[i] = xgb.transform_prediction(out_predictions[i]);
  }

  return out_predictions;
}

} // namespace kphp_ml_xgboost
