// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/kml/kphp_ml_interface.h"

#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"
#include "runtime-common/stdlib/kml/kphp_ml_catboost.h"
#include "runtime-common/stdlib/kml/kphp_ml_init.h"
#include "runtime-common/stdlib/kml/kphp_ml_xgboost.h"

Optional<array<double>> f$kml_xgboost_predict_matrix(const string& model_name, const array<array<double>>& features_map_matrix) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  if (!p_kml->is_xgboost()) {
    php_warning("called for non-xgboost model %s", model_name.c_str());
    return {};
  }
  if (!features_map_matrix.is_vector()) {
    php_warning("expecting a vector of hashmaps, but hashmap given");
    return {};
  }

  std::byte* mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_xgboost::kml_predict_xgboost(*p_kml, features_map_matrix, mutable_buffer);
}

Optional<double> f$kml_catboost_predict_vectors(const string& model_name, const array<double>& float_features, const array<string>& cat_features) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost()) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }
  if (p_kml->is_catboost_multi_classification()) {
    php_warning("called for MULTI-classification model %s", model_name.c_str());
    return {};
  }
  if (!float_features.is_vector() || !cat_features.is_vector()) {
    php_warning("expecting two vectors, but hashmap given");
    return {};
  }

  std::byte* mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_vectors(*p_kml, float_features, cat_features, mutable_buffer);
}

Optional<double> f$kml_catboost_predict_ht(const string& model_name, const array<double>& features_map) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost()) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }
  if (p_kml->is_catboost_multi_classification()) {
    php_warning("called for MULTI-classification model %s", model_name.c_str());
    return {};
  }
  if (features_map.is_vector()) {
    php_warning("expecting a hashmap, but vector given");
    return {};
  }

  std::byte* mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_ht_remap_str_keys(*p_kml, features_map, mutable_buffer);
}

Optional<array<double>> f$kml_catboost_predict_vectors_multi(const string& model_name, const array<double>& float_features, const array<string>& cat_features) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost()) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost_multi_classification()) {
    php_warning("called for NOT MULTI-classification model %s", model_name.c_str());
    return {};
  }
  if (!float_features.is_vector() || !cat_features.is_vector()) {
    php_warning("expecting two vectors, but hashmap given");
    return {};
  }

  std::byte* mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_vectors_multi(*p_kml, float_features, cat_features, mutable_buffer);
}

Optional<array<double>> f$kml_catboost_predict_ht_multi(const string& model_name, const array<double>& features_map) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost()) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }
  if (!p_kml->is_catboost_multi_classification()) {
    php_warning("called for NOT multi-classification model %s", model_name.c_str());
    return {};
  }
  if (features_map.is_vector()) {
    php_warning("expecting a hashmap, but vector given");
    return {};
  }

  std::byte* mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_ht_remap_str_keys_multi(*p_kml, features_map, mutable_buffer);
}

bool f$kml_model_exists(const string& model_name) {
  return kphp_ml_find_loaded_model_by_name(model_name) != nullptr;
}

array<string> f$kml_available_models() {
  const auto& kml_models_context = KmlModelsContext::get();

  array<string> response;
  response.reserve(kml_models_context.loaded_models.size(), true);
  for (const auto& [_, model] : kml_models_context.loaded_models) {
    response.push_back(string(model.model_name.c_str()));
  }
  return response;
}

Optional<array<string>> f$kml_get_feature_names(const string& model_name) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }
  const auto& feature_names = p_kml->get_feature_names();

  array<string> response;
  response.reserve(feature_names.size(), true);

  for (const kphp_ml::stl::string& feature_name : feature_names) {
    response.emplace_back(feature_name.c_str());
  }

  return response;
}

Optional<string> f$kml_get_custom_property(const string& model_name, const string& property_name) {
  const kphp_ml::MLModel* p_kml = kphp_ml_find_loaded_model_by_name(model_name);
  if (p_kml == nullptr) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  auto inner_result = p_kml->get_custom_property(kphp_ml::stl::string(property_name.c_str()));

  if (!inner_result.has_value()) {
    return {};
  }
  return string(inner_result.value().c_str());
}
