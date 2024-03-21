#include "runtime/kphp_ml/kphp_ml_interface.h"

#include "runtime/kphp_core.h"
#include "runtime/kphp_ml/kphp_ml.h"
#include "runtime/kphp_ml/kphp_ml_init.h"
#include "runtime/kphp_ml/kphp_ml_xgboost.h"
#include "runtime/kphp_ml/kphp_ml_catboost.h"

array<double> f$kml_xgboost_predict_matrix(const string &model_name, const array<array<double>> &features_map_matrix) {
  const kphp_ml::MLModel *p_kml = kphp_ml_find_loaded_model_by_name(model_name);
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

  char *mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_xgboost::kml_predict_xgboost(*p_kml, features_map_matrix, mutable_buffer);
}

double f$kml_catboost_predict_vectors(const string &model_name, const array<double> &float_features, const array<string> &cat_features) {
  const kphp_ml::MLModel *p_kml = kphp_ml_find_loaded_model_by_name(model_name);
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

  char *mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_vectors(*p_kml, float_features, cat_features, mutable_buffer);
}

double f$kml_catboost_predict_ht(const string &model_name, const array<double> &features_map) {
  const kphp_ml::MLModel *p_kml = kphp_ml_find_loaded_model_by_name(model_name);
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

  char *mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_ht_remap_str_keys(*p_kml, features_map, mutable_buffer);
}

array<double> f$kml_catboost_predict_vectors_multi(const string &model_name, const array<double> &float_features, const array<string> &cat_features) {
  const kphp_ml::MLModel *p_kml = kphp_ml_find_loaded_model_by_name(model_name);
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

  char *mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_vectors_multi(*p_kml, float_features, cat_features, mutable_buffer);
}

array<double> f$kml_catboost_predict_ht_multi(const string &model_name, const array<double> &features_map) {
  const kphp_ml::MLModel *p_kml = kphp_ml_find_loaded_model_by_name(model_name);
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

  char *mutable_buffer = kphp_ml_get_mutable_buffer_in_current_worker();
  return kphp_ml_catboost::kml_predict_catboost_by_ht_remap_str_keys_multi(*p_kml, features_map, mutable_buffer);
}
