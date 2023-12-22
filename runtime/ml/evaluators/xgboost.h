#pragma once

#include <string>

#include "runtime/kphp_core.h"
#include "runtime/ml/kml-files-reader.h"
#include "runtime/ml/kphp_ml.h"

class EvalXgboost {
  kphp_ml::MLModel &model;

public:
  explicit EvalXgboost(kphp_ml::MLModel &model) noexcept
    : model(model) {
    assert(model.model_kind == kphp_ml::ModelKind::xgboost_trees_no_cat);
    assert(model.input_kind == kphp_ml::InputKind::ht_direct_int_keys_to_fvalue || model.input_kind == kphp_ml::InputKind::ht_remap_int_keys_to_fvalue
           || model.input_kind == kphp_ml::InputKind::ht_remap_str_keys_to_fvalue);
  }

  array<double> predict_input(const array<array<double>> &features) const;
};
