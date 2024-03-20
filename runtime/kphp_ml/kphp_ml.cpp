#include "runtime/kphp_ml/kphp_ml.h"

bool kphp_ml::MLModel::is_catboost_multi_classification() const {
  if (!is_catboost()) {
    return false;
  }

  const auto &cbm = std::get<kphp_ml_catboost::CatboostModel>(impl);
  return cbm.dimension != -1 && !cbm.leaf_values_vec.empty();
}

unsigned int kphp_ml::MLModel::calculate_mutable_buffer_size() const {
  switch (model_kind) {
    case ModelKind::xgboost_trees_no_cat: {
      const auto &xgb = std::get<kphp_ml_xgboost::XgboostModel>(impl);
      return kphp_ml_xgboost::BATCH_SIZE_XGB * xgb.num_features_present * 2 * sizeof(float);
    }
    case ModelKind::catboost_trees: {
      const auto &cbm = std::get<kphp_ml_catboost::CatboostModel>(impl);
      return cbm.cat_feature_count * sizeof(string) +
             cbm.float_feature_count * sizeof(float) +
             (cbm.binary_feature_count + 4 - 1) / 4 * 4 + // round up to 4 bytes
             cbm.cat_feature_count * sizeof(int) +
             cbm.model_ctrs.used_model_ctrs_count * sizeof(float);
    }
    default:
      return 0;
  }
}
