// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/ml/kphp_ml.h"

#include <unordered_map> // TODO maybe change on better one?

array<double> f$predictXgb(const string &kml_path, const array<array<double>> & features);
array<double> f$predictCatboost(const string &kml_path, const array<array<double>> &float_feats, const array<array<string>> &string_feats);
void f$unsafeInitModels(const array<string> &kml_files);

extern std::unordered_map<std::string, kphp_ml::MLModel> LoadedModels;
extern std::string KmlDirPath;
extern size_t PredictionBufferSize;
extern char * PredictionBuffer;