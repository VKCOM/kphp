// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/kml/catboost.h"
#include "runtime-common/stdlib/kml/xgboost.h"
#include "runtime/kml.h"

inline Optional<array<double>> f$kml_xgboost_predict_matrix(const string& model_name, const array<array<double>>& features_map_matrix) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model not found: %s", model_name.c_str());
    return {};
  }

  const auto& kml{(*opt_model).get()};
  const auto opt_xgb{kml.as_xgboost()};
  if (!opt_xgb) {
    php_warning("called for non-xgboost model %s", model_name.c_str());
    return {};
  }

  if (!features_map_matrix.is_vector()) {
    php_warning("expecting a vector of hashmaps, but hashmap given");
    return {};
  }

  const auto& xgb{(*opt_xgb).get()};
  return kphp::kml::xgboost::predict(xgb, kml.input_kind(), features_map_matrix, KmlInferenceState::get().m_buffer.data());
}

inline Optional<double> f$kml_catboost_predict_vectors(const string& model_name, const array<double>& float_features,
                                                       const array<string>& cat_features) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model not found: %s", model_name.c_str());
    return {};
  }

  const auto& kml{(*opt_model).get()};
  const auto opt_cbm{kml.as_catboost()};
  if (!opt_cbm) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }

  const auto& cbm{(*opt_cbm).get()};
  if (cbm.is_multi_classification()) {
    php_warning("called for MULTI-classification model %s", model_name.c_str());
    return {};
  }

  if (!float_features.is_vector() || !cat_features.is_vector()) {
    php_warning("expecting two vectors, but hashmap given");
    return {};
  }

  return kphp::kml::catboost::predict_by_vectors(cbm, {model_name.c_str(), model_name.size()}, float_features, cat_features,
                                                 KmlInferenceState::get().m_buffer.data());
}

inline Optional<array<double>> f$kml_catboost_predict_vectors_multi(const string& model_name, const array<double>& float_features,
                                                                    const array<string>& cat_features) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  const auto& kml{(*opt_model).get()};
  const auto opt_cbm{kml.as_catboost()};
  if (!opt_cbm) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }

  const auto& cbm{(*opt_cbm).get()};
  if (!cbm.is_multi_classification()) {
    php_warning("called for NOT MULTI-classification model %s", model_name.c_str());
    return {};
  }

  if (!float_features.is_vector() || !cat_features.is_vector()) {
    php_warning("expecting two vectors, but hashmap given");
    return {};
  }

  return kphp::kml::catboost::predict_by_vectors_multi(cbm, {model_name.c_str(), model_name.size()}, float_features, cat_features,
                                                       KmlInferenceState::get().m_buffer.data());
}

inline Optional<double> f$kml_catboost_predict_ht(const string& model_name, const array<double>& features_map) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  const auto& kml{(*opt_model).get()};
  const auto opt_cbm{kml.as_catboost()};
  if (!opt_cbm) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }

  const auto& cbm{(*opt_cbm).get()};
  if (cbm.is_multi_classification()) {
    php_warning("called for MULTI-classification model %s", model_name.c_str());
    return {};
  }

  if (features_map.is_vector()) {
    php_warning("expecting a hashmap, but vector given");
    return {};
  }

  return kphp::kml::catboost::predict_by_ht_remap_str_keys(cbm, features_map, KmlInferenceState::get().m_buffer.data());
}

inline Optional<array<double>> f$kml_catboost_predict_ht_multi(const string& model_name, const array<double>& features_map) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  const auto& kml{(*opt_model).get()};
  const auto opt_cbm{kml.as_catboost()};
  if (!opt_cbm) {
    php_warning("called for non-catboost model %s", model_name.c_str());
    return {};
  }

  const auto& cbm{(*opt_cbm).get()};
  if (!cbm.is_multi_classification()) {
    php_warning("called for NOT MULTI-classification model %s", model_name.c_str());
    return {};
  }

  if (features_map.is_vector()) {
    php_warning("expecting a hashmap, but vector given");
    return {};
  }

  return kphp::kml::catboost::predict_by_ht_remap_str_keys_multi(cbm, features_map, KmlInferenceState::get().m_buffer.data());
}

inline bool f$kml_model_exists(const string& model_name) noexcept {
  return KmlModelsState::get().find_model({model_name.c_str(), model_name.size()}).has_value();
}

inline array<string> f$kml_available_models() noexcept {
  const auto& models{KmlModelsState::get().models()};

  array<string> res{};
  res.reserve(models.size(), true);
  for (const auto& [_, model] : models) {
    res.push_back(string{model.name().data(), static_cast<string::size_type>(model.name().size())});
  }

  return res;
}

inline Optional<array<string>> f$kml_get_feature_names(const string& model_name) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  const auto& feature_names{(*opt_model).get().feature_names()};

  array<string> res{};
  res.reserve(feature_names.size(), true);
  for (const auto& feature_name : feature_names) {
    res.push_back(string{feature_name.c_str(), static_cast<string::size_type>(feature_name.size())});
  }

  return res;
}

inline Optional<string> f$kml_get_custom_property(const string& model_name, const string& property_name) noexcept {
  auto opt_model{KmlModelsState::get().find_model({model_name.c_str(), model_name.size()})};
  if (!opt_model) {
    php_warning("kml model %s not found", model_name.c_str());
    return {};
  }

  const auto opt_property{(*opt_model).get().custom_property({property_name.c_str(), property_name.size()})};
  if (!opt_property) {
    return {};
  }

  const auto& property{(*opt_property)};
  return string{property.data(), static_cast<string::size_type>(property.size())};
}
