#pragma once

#include <string>

#include "runtime/kphp_core.h"
#include "runtime/ml/interface.h"
#include "runtime/ml/internals.h"
#include "runtime/ml/kml-files-reader.h"

class EvalXgboost {
  ml_internals::MLModel &model;

public:
  explicit EvalXgboost(ml_internals::MLModel &model) noexcept
    : model(model) {
    assert(model.model_kind == ml_internals::ModelKind::xgboost_trees_no_cat);
    assert(model.input_kind == ml_internals::InputKind::ht_direct_int_keys_to_fvalue || model.input_kind == ml_internals::InputKind::ht_remap_int_keys_to_fvalue
           || model.input_kind == ml_internals::InputKind::ht_remap_str_keys_to_fvalue);
  }

  array<double> predict_input(const array<array<double>> &features) const;
};
