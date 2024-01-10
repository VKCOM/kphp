// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/ml/ml.h"
#include "runtime/ml/evaluators/catboost.h"
#include "runtime/ml/evaluators/xgboost.h"

// TODO from to std::string to our ::string
// For now there are some problems with std::hash

std::unordered_map<std::string, kphp_ml::MLModel> LoadedModels;
std::string KmlDirPath;
size_t PredictionBufferSize;
char * PredictionBuffer = nullptr;



void f$unsafeInitModels(const array<string> &kml_files) {
  for (const auto &kml_file : kml_files) {
    const auto &file = kml_file.get_value();
    if (LoadedModels.count(std::string(file.c_str())) == 0) {
      LoadedModels[std::string(file.c_str())] = kml_file_read(file.c_str());
    }
  }
}

array<double> f$predictXgb(const string &kml_path, const array<array<double>> &features) {
  const auto &kml_iter = LoadedModels.find(std::string(kml_path.c_str()));
  if (kml_iter == LoadedModels.end()) {
    return {};
  }

  EvalXgboost evaluator(kml_iter->second);

  return evaluator.predict_input(features);
}

array<double> f$predictCatboost(const string &kml_path, const array<array<double>> &float_feats, const array<array<string>> &string_feats) {
  const auto &kml_iter = LoadedModels.find(std::string(kml_path.c_str()));
  if (kml_iter == LoadedModels.end()) {
    return {};
  }

  EvalCatboost evaluator(kml_iter->second);

  return evaluator.predict_input(float_feats, string_feats);
}
