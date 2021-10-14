// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <xgboost/learner.h>

#include "runtime/xgboost/predict.h"
#include "runtime/xgboost/model.h"

array<double> f$xgboost_predict_experimental(const array<array<double>> &features) try {
  static xgboost::PredictionCacheEntry cache_scores;
  vk::singleton<vk::xgboost::Model>::get().predict(features, cache_scores);

  const auto &scores = cache_scores.predictions.ConstHostVector();
  assert(features.size().int_size == scores.size());

  array<double> kphp_scores;
  kphp_scores.reserve(scores.size(), 0, false);
  std::size_t idx = 0;
  for (const auto &id : features) {
    kphp_scores.emplace_value(id.get_int_key(), static_cast<double>(scores[idx++]));
  }
  return kphp_scores;
} catch (const std::exception &e) {
  php_warning("exception in xgboost library: %s", e.what());
  return {};
}
