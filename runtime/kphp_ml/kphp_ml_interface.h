// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

Optional<array<double>> f$kml_xgboost_predict_matrix(const string& model_name, const array<array<double>>& features_map_matrix);

Optional<double> f$kml_catboost_predict_vectors(const string& model_name, const array<double>& float_features, const array<string>& cat_features);
Optional<double> f$kml_catboost_predict_ht(const string& model_name, const array<double>& features_map);

Optional<array<double>> f$kml_catboost_predict_vectors_multi(const string& model_name, const array<double>& float_features, const array<string>& cat_features);
Optional<array<double>> f$kml_catboost_predict_ht_multi(const string& model_name, const array<double>& features_map);

bool f$kml_model_exists(const string& model_name);

Optional<array<string>> f$kml_get_feature_names(const string& model_name);
Optional<string> f$kml_get_custom_property(const string& model_name, const string& property_name);
