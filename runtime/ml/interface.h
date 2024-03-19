// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/ml/internals.h"

#include <unordered_map> // TODO maybe change on better one?

array<double> f$kml_xgb_predict(const string &model_name, const array<array<double>> & features);
array<double> f$kml_catboost_predict(const string &model_name, const array<array<double>> &float_feats, const array<array<string>> &string_feats);
array<array<double>> f$kml_catboost_predict_multi(const string &model_name, const array<array<double>> &float_feats, const array<array<string>> &string_feats);


extern std::unordered_map<std::string, ml_internals::MLModel> LoadedModels;
extern std::string KmlDirPath;
extern size_t PredictionBufferSize;
extern char * PredictionBuffer;