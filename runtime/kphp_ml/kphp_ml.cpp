// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// ATTENTION!
// This file exists both in KPHP and in a private vkcom repo "ml_experiments".
// They are almost identical, besides include paths and input types (`array` vs `unordered_map`).

#include "runtime/kphp_core.h"
#include "runtime/kphp_ml/kphp_ml.h"

// for detailed comments about KML, see kphp_ml.h

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

const std::vector<std::string> &kphp_ml::MLModel::get_feature_names() const {
  return feature_names;
}

std::optional<std::string> kphp_ml::MLModel::get_custom_property(const std::string &property_name) const {
  auto it = custom_properties.find(property_name);
  if (it == custom_properties.end()) {
    return std::nullopt;
  }
  return it->second;
}
