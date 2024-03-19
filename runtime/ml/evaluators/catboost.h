#pragma once

#include <string>

#include "runtime/kphp_core.h"
#include "runtime/ml/internals.h"
#include "runtime/ml/kml-files-reader.h"

class EvalCatboost {
  ml_internals::MLModel model;

public:
  explicit EvalCatboost(const ml_internals::MLModel &model) noexcept
    : model(model) {
    assert(model.model_kind == ml_internals::ModelKind::catboost_trees);
    assert(model.input_kind == ml_internals::InputKind::vectors_float_and_categorial ||
           model.input_kind == ml_internals::InputKind::vectors_float_and_categorial_multi);
  }

  array<double> predict_input(const array<array<double>> &float_features, const array<array<string>> &cat_features) const;
  array<array<double>> predict_input_multi(const array<array<double>> &float_features, const array<array<string>> &cat_features) const;
};
