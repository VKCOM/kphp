#pragma once

#include <string>

#include "runtime/kphp_core.h"
#include "runtime/ml/kml-files-reader.h"
#include "runtime/ml/kphp_ml.h"

class EvalCatboost {
  kphp_ml::MLModel model;

public:
  explicit EvalCatboost(const kphp_ml::MLModel &model) noexcept
    : model(model) {
    assert(model.model_kind == kphp_ml::ModelKind::catboost_trees);
    assert(model.input_kind == kphp_ml::InputKind::vectors_float_and_categorial);
  }

  array<double> predict_input(const array<array<double>> &float_features, const array<array<string>> &cat_features) const;
};
