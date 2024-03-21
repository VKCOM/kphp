#pragma once

#include "runtime/kphp_core.h"

array<double> f$kml_xgboost_predict_matrix(const string &model_name, const array<array<double>> &features_map_matrix);

double f$kml_catboost_predict_vectors(const string &model_name, const array<double> &float_features, const array<string> &cat_features);
double f$kml_catboost_predict_ht(const string &model_name, const array<double> &features_map);

array<double> f$kml_catboost_predict_vectors_multi(const string &model_name, const array<double> &float_features, const array<string> &cat_features);
array<double> f$kml_catboost_predict_ht_multi(const string &model_name, const array<double> &features_map);
