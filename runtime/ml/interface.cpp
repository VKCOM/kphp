// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/ml/interface.h"
#include "runtime/ml/evaluators/catboost.h"
#include "runtime/ml/evaluators/xgboost.h"

// TODO from to std::string to our ::string
// For now there are some problems with std::hash

std::unordered_map<std::string, ml_internals::MLModel> LoadedModels;
std::string KmlDirPath;
size_t PredictionBufferSize;
char *PredictionBuffer = nullptr;

array<double> f$kml_xgb_predict(const string &model_name, const array<array<double>> &features) {
  const auto &kml_iter = LoadedModels.find(std::string(model_name.c_str()));
  if (kml_iter == LoadedModels.end()) {
    return {};
  }

  EvalXgboost evaluator(kml_iter->second);

  return evaluator.predict_input(features);
}

array<double> f$kml_catboost_predict(const string &model_name, const array<array<double>> &float_feats, const array<array<string>> &string_feats) {
  const auto &kml_iter = LoadedModels.find(std::string(model_name.c_str()));
  if (kml_iter == LoadedModels.end()) {
    return {};
  }

  EvalCatboost evaluator(kml_iter->second);

  return evaluator.predict_input(float_feats, string_feats);
}
